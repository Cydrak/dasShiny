struct Browser : Window {
  VerticalLayout layout;
  HorizontalLayout pathLayout;
    LineEdit pathEdit;
    Button homeButton;
    Button upButton;
  ListView fileList;
  HorizontalLayout controlLayout;
  Label filterLabel;
  Button openButton;

  string select(const string &title);
  string select(const string &title, const string &extension);
  void saveConfiguration();
  void synchronize();
  void bootstrap();
  Browser();

private:
  configuration config;
  struct Folder {
    string extension;
    string path;
    unsigned selection;
  };
  vector<Folder> folderList;
  Folder import;

  string outputFilename;

  string extension;
  string path;
  lstring filenameList;

  void setPath(const string &path, unsigned selection = 0);
  void fileListActivate();
};

extern Browser *browser;
