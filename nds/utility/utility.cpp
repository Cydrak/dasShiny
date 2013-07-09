#include "utility.hpp"

namespace NintendoDS {

using namespace nall;
#include "sha1.cpp"

static string hexfrombase64(string s) {
  string result;
  uint8_t *data; unsigned size;
  if(!base64::decode(data, size, s)) return "";
  
  for(unsigned n = 0; n < size; n++) 
    result.append(hex<2>(data[n]));
  
  delete[] data;
  return result;
}


// game id => manifest
static nall::map<string, nall::vector<string>> database;


void loadDatabase(string markup) {
  lstring items = markup.split("\n\n");
  
  for(auto &item : items) {
    auto elem = Markup::Document(item);
    if(auto gid  = elem["title/id"].text()) {
      // Maybe keep this sorted by region for approximate matches...
      database(substr(gid, 0, 3)).append(item);
    }
  }
}

static string heuristicManifest(
  string name, string gid, string save, uint8_t *data, unsigned size)
{
  string romSize{size};
  if(size>>10) romSize = {size>>10,"K"};
  if(size>>20) romSize = {size>>20,"M"};
  if(size>>30) romSize = {size>>30,"G"};
  
  // Alternately we could use the game's banner.
  string manifest{"title:",name,"\n"};
  manifest.append("  id=",gid);
  manifest.append(" rom=",romSize);
  
  vector<string> pokemon{
    "ADA", "APA", "CPU",         // Diamond/Pearl/Platinum
    "IPK", "IPG",                // HeartGold/SoulSilver
    "IRB", "IRA", "IRE", "IRD"   // Black/White[2]
  };
  
  if(gid.beginswith("V") || gid.beginswith("IR")) {
    print("heuristic: DSi support.\n");
    manifest.append(" dsi=en");
  }
  if(gid.beginswith("I")) {
    print("heuristic: Infrared card.\n");
    manifest.append(" infrared");
  }
  
  if(gid.beginswith("UOR")) {
    print("heuristic: NAND card.\n");       // WarioWare D.I.Y. / Made in Ore
    manifest.append(" nand=32M");
  }
  else if(gid.beginswith("UXB")) {
    print("heuristic: 8M flash card.\n");   // Band Bros. DX
    manifest.append(" flash=8M");
  }
  else if(gid.beginswith("UBR")) {
    print("heuristic: Opera browser.\n");   // Nintendo DS Browser
    manifest.append(" browser flash=256K");
  }
  else if(gid.beginswith("UZP")) {
    print("heuristic: Bluetooth card.\n");  // Pokemon Typing - Battle & Get!
    manifest.append(" bluetooth");
  }
  else if(pokemon.find(substr(gid,0,3))) {
    // Special check to stave off forum threads!
    // All these use 512K of flash, so that's nice.
    print("heuristic: Pokemon title.\n");
    manifest.append(" flash=512K");
  }
  else {
    manifest.append(" ",save);
  }
  manifest.append("\n  heuristic\n");
  return manifest;
}

string findManifest(uint8_t *data, unsigned size, string name, bool hash) {
  if(size < 0x1000) return "";
  
  char id[5] = {};
  memcpy(id, data+12, 4);
  string gid = id;
  
  // Look up based on first three letters of the game serial.
  // The fourth depends on region.
  bool found = false;
  auto list = database(substr(gid, 0, 3));
  string save;
  string actualSha1;
  string actualSha256;
  
  // First try an exact match.
  for(auto title : list) {
    auto   elem     = Markup::Document(title);
    string dbId     = elem["title/id"].text();
    string dbSha1   = hexfrombase64(elem["title/dec/sha1"].text());
    string dbSha256 = hexfrombase64(elem["title/dec/sha256"].text());
    
    // Even if we don't find one, keep the save type in mind.
    // It's likely to match another region of the same title.
    if(elem["title/id/eeprom"].exists()) save = {"eeprom=",elem["title/id/eeprom"].text()};
    if(elem["title/id/flash"].exists())  save = {"flash=",elem["title/id/flash"].text()};
    if(elem["title/id/nand"].exists())   save = {"nand=",elem["title/id/nand"].text()};
    
    if(gid == dbId) {
      print("Matched ID ",gid,"\n");
      
      if(hash == false) {
        return title;
      }
      else if(dbSha256) {
        if(!actualSha256) actualSha256 = sha256(data, size);
        
        print("  SHA256: ",dbSha256,"\n");
        print("  Actual: ",actualSha256,"\n");
        
        if(dbSha256 == actualSha256)
          return title;
      }
      else if(dbSha1) {
        if(!actualSha1) actualSha1 = sha1(data, size);
        
        print("  SHA1:   ",dbSha1,"\n");
        print("  Actual: ",actualSha1,"\n");
        
        if(dbSha1 == actualSha1)
          return title;
      }
    }
  }
  return heuristicManifest(name, gid, save, data, size);
}

string findManifest(string imagePath, string name, bool hash) {
  filemap image; image.open(imagePath, filemap::mode::read);
  if(!name) name = basename(notdir(imagePath));
  return findManifest(image.data(), image.size(), name, hash);
}

void printNode(const Markup::Node &n, int level = 0) {
  for(int i = level; i > 0; i--)
    print("  ");
  
  print(n.name,":",n.data,"\n");
  for(auto &c : n)
    printNode(c, level + 1);
}

bool importROMImage(string& container, string libraryPath, string sourcePath) {
  string manifest = findManifest(sourcePath, "", true);
  auto elem = Markup::Document(manifest);
  
  print("Manifest:\n",manifest,"\n");
  print("Tree:\n"); printNode(elem);
  
  string name   = elem["title"].text();      // Title "The Legend of Foo"
  string folder = elem["title/file"].text(); //   eg. "Legend of Foo, The"
  string id     = elem["title/id"].text();
  
  if(!folder) folder = name;
  
  container = {libraryPath,folder,".nds/"};
  string romImage = {container,"rom"};
  directory::create(container);  // return value is not working atm
  
  // It's quite possible to run out of disk space copying DS games around..
  if(!file::copy(sourcePath, romImage)
    || file::size(sourcePath) != file::size(romImage))
  {
    file::remove(romImage);
    return false;
  }
  file mf;
  mf.open({container,"manifest.bml"}, file::mode::write);
  
  string dsi       = elem["title/id/dsi"].text();
  string country   = elem["title/id/country"].text();
  string langs     = elem["title/id/langs"].text();
  string rom       = elem["title/id/rom"].text();
  string esha1     = elem["title/enc/sha1"].text();
  string dsha1     = elem["title/dec/sha1"].text();
  string nand      = elem["title/id/nand"].text();
  string flash     = elem["title/id/flash"].text();
  string eeprom    = elem["title/id/eeprom"].text();
  bool   browser   = elem["title/id/browser"].exists();
  bool   infrared  = elem["title/id/infrared"].exists();
  bool   bluetooth = elem["title/id/bluetooth"].exists();
  
  mf.print("title: ",name,"\n");
  
  if(id)        mf.print("  id:         ",id,"\n");
  if(dsi=="en") mf.print("  dsi:        supported\n");
  if(country)   mf.print("  country:    ",country,"\n");
  if(langs)     mf.print("  languages:  ",langs,"\n");
  mf.print("\n");
  if(rom)       mf.print("  rom size=",rom," data:rom\n");
  if(esha1)     mf.print("    encrypted sha1: ",hexfrombase64(esha1),"\n");
  if(dsha1)     mf.print("    decrypted sha1: ",hexfrombase64(dsha1),"\n");
  mf.print("\n");
  if(nand)      mf.print("  nand size=",nand," data:nand\n");
  if(flash)     mf.print("  flash size=",flash," data:flash\n");
  if(eeprom)    mf.print("  eeprom size=",eeprom," data:eeprom\n");
  if(browser)   mf.print("  browser\n");
  if(infrared)  mf.print("  infrared\n");
  if(bluetooth) mf.print("  bluetooth\n");
  mf.print("\n");
  return true;
}

}
