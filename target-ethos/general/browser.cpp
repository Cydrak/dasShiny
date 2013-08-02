Browser *browser = nullptr;

Browser::Browser() {
  bootstrap();
  setGeometry({128, 128, 640, 400});
  windowManager->append(this, "Browser");

  layout.setMargin(5);
  homeButton.setImage({resource::home, sizeof resource::home});
  upButton.setImage({resource::up, sizeof resource::up});
  openButton.setText("Open");

  append(layout);
  layout.append(pathLayout, {~0, 0}, 5);
    pathLayout.append(pathEdit, {~0, 0}, 5);
    pathLayout.append(homeButton, {28, 28}, 5);
    pathLayout.append(upButton, {28, 28});
  layout.append(fileList, {~0, ~0}, 5);
  layout.append(controlLayout, {~0, 0});
    controlLayout.append(filterLabel, {~0, 0}, 5);
    controlLayout.append(openButton, {80, 0});

  pathEdit.onActivate = [&] {
    string path = pathEdit.text();
    path.transform("\\", "/");
    if(path.endswith("/") == false) path.append("/");
    setPath(path);
  };

  homeButton.onActivate = [&] {
    setPath(utility->libraryPath());
  };

  upButton.onActivate = [&] {
    setPath(parentdir(path));
  };

  fileList.onChange = {&Browser::synchronize, this};
  fileList.onActivate = openButton.onActivate = {&Browser::fileListActivate, this};

  onClose = [&] {
    setModal(false);
    setVisible(false);
  };

  synchronize();
}

void Browser::synchronize() {
  openButton.setEnabled(fileList.selected());
  if(fileList.selected()) {
    for(auto &folder : folderList) {
      if(folder.extension == extension) {
        folder.selection = fileList.selection();
      }
    }
  }
}

void Browser::saveConfiguration() {
  config.save(program->savePath("paths.cfg"));
}

void Browser::bootstrap() {
  for(auto &emulator : program->emulator) {
    for(auto &media : emulator->media) {
      bool found = false;
      for(auto &folder : folderList) {
        if(folder.extension == media.type) {
          found = true;
          break;
        }
      }
      if(found == true) continue;

      Folder folder;
      folder.extension = media.type;
      folder.path = {userpath(), "Emulation/", media.name, "/"};
      folder.selection = 0;
      folderList.append(folder);
    }
  }
  import.path = userpath();
  import.selection = 0;
  
  for(auto &folder : folderList) {
    config.append(folder.path, folder.extension);
    config.append(folder.selection, string{folder.extension, "::selection"});
  }
  config.append(import.path, "Import");
  config.append(import.path, "Import::selection");

  config.load(program->loadPath("paths.cfg"));
  config.save(program->savePath("paths.cfg"));
}

string Browser::select(const string& title) {
  this->extension = "";

  setPath(import.path? import.path : program->basepath, import.selection);
  
  string extensions;
  for(auto &folder : folderList)
    extensions.append("; *.", folder.extension);
  extensions.ltrim("; ");
  filterLabel.setText({"Filter: ",extensions});

  audio.clear();
  setTitle(title);
  setVisible(true);
  fileList.setFocused();
  outputFilename = "";

  setModal();
  return outputFilename;
}

string Browser::select(const string &title, const string &extension) {
  this->extension = extension;

  string path;
  unsigned selection = 0;
  for(auto &folder : folderList) {
    if(folder.extension == extension) {
      path = folder.path;
      selection = folder.selection;
      break;
    }
  }
  if(path.empty()) path = program->basepath;
  setPath(path, selection);

  filterLabel.setText({"Filter: *.", extension});

  audio.clear();
  setTitle(title);
  setVisible(true);
  fileList.setFocused();
  outputFilename = "";

  setModal();
  return outputFilename;
}

image imageFromIcon(const string& path) {
  image defaultIcon{resource::game, sizeof resource::game};
  file fp;
  if(!fp.open(path, file::mode::read)) return defaultIcon;
  if(fp.readl(4) != 0x10000) return defaultIcon;
  
  // Search directory for the biggest, highest depth icon.
  image icon;
  unsigned width = 0, height = 0, bpp = 0, offset = 0, size = 0;
  unsigned dirCount = fp.readl(2);
  while(dirCount--) {
    unsigned curWidth = fp.readl(1);   if(!curWidth)  curWidth = 256;
    unsigned curHeight = fp.readl(1);  if(!curHeight) curHeight = 256;
    fp.readl(4);
    unsigned curBpp = fp.readl(2);
    unsigned curSize = fp.readl(4);
    unsigned curOffset = fp.readl(4);
    
    if(curWidth*curHeight > width*height || curBpp > bpp) {
      width = curWidth;
      height = curHeight;
      bpp = curBpp;
      offset = curOffset;
      size = curSize;
    }
  }
  fp.seek(offset);
  
  uint32 bmiSize = fp.readl(4);
  int32  bmiWidth = fp.readl(4);
  int32  bmiHeight = fp.readl(4);
  uint16 bmiPlanes = fp.readl(2);
  uint16 bmiDepth = fp.readl(2);
  
  // Indexed and PNG icons aren't presently supported, since
  //   NintendoDS::importROMImage always outputs ARGB8888.
  bool valid = bmiSize != 0x474e5089 && bmiSize >= 40
    && bmiWidth == width && bmiHeight == 2*height
    && bmiPlanes == 1 && bmiDepth == bpp;
  
  if(!valid || bpp < 32) return defaultIcon;
  
  fp.seek(offset + bmiSize);
  icon.allocate(width, height);
  for(unsigned y = 0; y < height; y++)
    fp.read(icon.data + icon.pitch*(height-1-y), width*4);
  
  return icon;
}

void Browser::setPath(const string &path, unsigned selection) {
  //save path for next browser selection
  if(this->extension) {
    for(auto &folder : folderList)
      if(folder.extension == extension)
        folder.path = path;
  }
  else {
    import.path = path;
  }

  this->path = path;
  pathEdit.setText(path);

  fileList.reset();
  filenameList.reset();

  lstring contents = directory::ifolders(path);
  
  // Normal folders
  for(auto &filename : contents) {
    string suffix = {".", this->extension, "/"};
    if(filename.endswith("/") && !filename.endswith(suffix)) {
      string name = filename;
      name.rtrim<1>("/");
      fileList.append(name);
      fileList.setImage(filenameList.size(), 0, {resource::folder, sizeof resource::folder});
      filenameList.append(filename);
    }
  }
  
  if(this->extension) {
    // Game folders
    string suffix = {".", this->extension, "/"};
    
    for(auto &filename : contents) {
      if(filename.endswith(suffix)) {
        string name = filename;
        name.rtrim<1>(suffix);
        fileList.append(name);
        //fileList.setImage(filenameList.size(), 0, {resource::game, sizeof resource::game});
        fileList.setImage(filenameList.size(), 0,
          imageFromIcon({path,filename,"banner.ico"}));
        filenameList.append(filename);
      }
    }
  }
  else {
    // Game files
    contents = directory::ifiles(path);
    
    for(auto &filename : contents) {
      for(auto &folder : folderList) {
        string suffix = {".", folder.extension};
        //if(filename.endswith(suffix)) {
          string name = filename;
          name.rtrim<1>(suffix);
          fileList.append(name);
          fileList.setImage(filenameList.size(), 0, {resource::unverified, sizeof resource::unverified});
          filenameList.append(filename);
        //}
      }
    }
  }

  fileList.setSelection(selection);
  fileList.setFocused();
  synchronize();
}

void Browser::fileListActivate() {
  unsigned selection = fileList.selection();
  string filename = filenameList[selection];
  
  bool select = false;
  
  if(!this->extension) {
    for(auto &folder : folderList)
      if(filename.endswith(string{".", folder.extension}))
        select = true;
    
    if(!filename.endswith("/"))
      select = true;
  }
  else if(string{filename}.rtrim<1>("/").endswith(this->extension)) {
    select = true;
  }
  
  if(!select) return setPath({path, filename});
  
  outputFilename = {path, filename};
  onClose();
}
