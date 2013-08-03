#if defined(_WIN32)
#define _WIN32_WINNT 0x0501
#define UNICODE
#define NOMINMAX
#include <winsock2.h>
#include <windows.h>
#include <shlwapi.h>
#endif

#include <nall/nall.hpp>
#include <nall/hashset.hpp>
#include "utility.hpp"

namespace NintendoDS {

using namespace nall;

#include "crc16.cpp"
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

static string utf8fromUcs2(uint8_t *data) {
  size_t len = 0;
  string s;
  for(;;) {
    unsigned code;
    code  = *data++ << 0;
    code += *data++ << 8;
    
    if(code > 0x7ff) {
      s.resize(len + 3);
      s[len++] = 0xe0|code>>12,
      s[len++] = 0x80|code>>6 & 0x3f,
      s[len++] = 0x80|code>>0 & 0x3f;
    }
    else if(code > 0x7f) {
      s.resize(len + 2);
      s[len++] = 0xc0|code>>6,
      s[len++] = 0x80|code>>0 & 0x3f;
    }
    else if(code > 0) {
      s.resize(len + 1);
      s[len++] = 0x00|code;
    }
    else {
      s[len] = 0; break;
    }
  }
  return s;
}

static void convertIcon(string outPath, uint8_t* data) {
  uint8_t* bitmap  = data + 0x000;  // 4x4 x 4bpp tiles
  uint8_t* palette = data + 0x200;  // 16 x RGB555
  
  uint32_t pixels[32*32];
  uint32_t outpal[16] = {0};
  
  for(int n = 1; n < 16; n++) {
    int color = palette[2*n+0] | palette[2*n+1]<<8;
    int r = (color>> 0 & 31) * 0x21/4;
    int g = (color>> 5 & 31) * 0x21/4;
    int b = (color>>10 & 31) * 0x21/4;
    
    outpal[n] = 0xff<<24 | r<<16 | g<<8 | b;
  }
  
  for(int ty = 0; ty < 4; ty++) {
    for(int tx = 0; tx < 4; tx++) {
      auto dest = pixels + 32*(31 - 8*ty) + 8*tx;
      auto src  = bitmap + 32*(4*ty + tx);
      
      for(int y = 0; y < 8; y++)
        for(int x = 0; x < 8; x++)
          dest[x - 32*y] = outpal[0xf & src[4*y + x/2] >> 4*(x%2)];
    }
  }
  
  unsigned rawsize = 32*32*4;
  file fp;
  fp.open(outPath, file::mode::write);
  
  // Icon header
  fp.writel(0x10000, 4);  // icon type
  fp.writel(1, 2);        // image count
  
  // Icon directory
  fp.writel(32, 1);  // width
  fp.writel(32, 1);  // height
  fp.writel(0,  1);  // ncolors
  fp.writel(0,  1);  // reserved
  fp.writel(1,  2);  // planes
  fp.writel(32, 2);  // bpp
  fp.writel(40 + rawsize, 4);
  fp.writel(22, 4);  // bmp offset
  
  //fp.write('B');
  //fp.write('M');
  //fp.writel(54 + rawsize, 4);  // bitmap size
  //fp.writel(0, 4);             // reserved
  //fp.writel(54, 4);            // pixel offset
  
  fp.writel(40, 4);            // info size
  fp.writel(32, 4);            //   width
  fp.writel(64, 4);            //   height <- must be 2x, even if mask omitted
  fp.writel(1, 2);             //   planes
  fp.writel(32, 2);            //   bpp
  fp.writel(0, 4);             //   compression
  fp.writel(rawsize, 4);       //   image size
  fp.writel(0, 4);             //   x ppm
  fp.writel(0, 4);             //   y ppm
  fp.writel(0, 4);
  fp.writel(0, 4);
  
  fp.write((uint8_t*)pixels, rawsize);
}



template<typename T, typename U> struct hashmap {
  struct node : T {
    U value;
    U& operator*() { return value; }
    node(const T& key) : T(key) {}
    node(const T& key, const U& value) : T(key), value(value) {}
  };
  void reset() { table.reset(); }
  void insert(const T& key, const U& value) { table.insert({key, value}); }
  void remove(const T& key) { table.remove({key}); }
  
  optional<U&> find(const T& key) {
    if(auto node = table.find({key})) return {true, **node};
    return false;
  }
  
  U& operator()(const T& key) {
    if(auto node = find({key})) return *node;
    return **table.insert({key, U()});
  }
  hashset<node> table;
};


//In instantiation of nall::optional<U&> hashmap<T, U>::find(const T&) const
//  passing const hashset<node_t> as this of
//  optional<T&> hashset<T>::find(const T&) [with T = node_t] discards qualifiers

// game id => manifest
static hashmap<string, nall::vector<string>> database;


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
  manifest.append("  id=",gid," rom=",romSize);
  
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
  
  if(pokemon.find(substr(gid,0,3))) {
    // Special check to stave off forum threads!
    // All these use 512K of flash, so that's nice.
    print("heuristic: Pokemon title.\n");
    manifest.append(" flash=512K");
  }
  else if(gid.beginswith("UOR")) {
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
  else if(save) {
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

// A quick sanity check, mostly so we don't import complete junk.
bool validateHeader(const stream& s) {
  auto rangeCheck = [&](
    uint32_t destOffset, uint32_t destSize,
    uint32_t srcOffset, uint32_t srcSize)
  -> bool {
    return srcOffset           - destOffset <= destSize
        && srcOffset + srcSize - destOffset <= destSize;
  };
  uint8_t headerData[0x170];
  
  s.seek(0x000);   s.read(headerData, 0x170);
  s.seek(0x014);   uint8_t chipSize = s.readl(1);
  s.seek(0x020);
  uint32_t arm9offset   = s.readl(4),  arm9entry  = s.readl(4);
  uint32_t arm9load     = s.readl(4),  arm9size   = s.readl(4);
  uint32_t arm7offset   = s.readl(4),  arm7entry  = s.readl(4);
  uint32_t arm7load     = s.readl(4),  arm7size   = s.readl(4);
  uint32_t fntOffset    = s.readl(4),  fntSize    = s.readl(4);
  uint32_t fatOffset    = s.readl(4),  fatSize    = s.readl(4);
  uint32_t arm9ovOffset = s.readl(4),  arm9ovSize = s.readl(4);
  uint32_t arm7ovOffset = s.readl(4),  arm7ovSize = s.readl(4);
  
  s.seek(0x068);   uint32_t bannerOffset = s.readl(4);
  s.seek(0x080);   uint32_t endOffset    = s.readl(4);
  s.seek(0x15c);
  uint16_t logoCrc      = s.readl(2);
  uint16_t headerCrc    = s.readl(2);
  
  // Some homebrew have garbage here..
  endOffset = s.size();
  
  return chipSize < 16 
    && arm7size >= 4 && arm9size >= 4
    && endOffset <= s.size()
    && endOffset <= (0x20000 << chipSize)
    && headerCrc == crc16(headerData + 0x000, 0x15e)
    && logoCrc   == crc16(headerData + 0x0c0, 0x09c)
    
    && (rangeCheck(0x200, endOffset, bannerOffset, 0x840) || !bannerOffset)
    && (rangeCheck(0x200, endOffset, fntOffset, fntSize) || !fntOffset && !fntSize)
    && (rangeCheck(0x200, endOffset, fatOffset, fatSize) || !fatOffset && !fatSize)
    && (rangeCheck(0x200, endOffset, arm9ovOffset, arm9ovSize) || !arm9ovOffset && !arm9ovSize)
    && (rangeCheck(0x200, endOffset, arm7ovOffset, arm7ovSize) || !arm7ovOffset && !arm7ovSize)
    &&  rangeCheck(0x200, endOffset, arm9offset, arm9size)
    &&  rangeCheck(0x200, endOffset, arm7offset, arm7size)
    
    &&  rangeCheck(0x02000000, 0x023bfe00, arm9load, arm9size)
    && (rangeCheck(0x02000000, 0x023bfe00, arm7load, arm7size)
     || rangeCheck(0x037f8000, 0x03807e00, arm7load, arm7size))
    
    &&  rangeCheck(0x02000000, 0x023bfe00, arm9entry, 4)
    && (rangeCheck(0x02000000, 0x023bfe00, arm7entry, 4)
     || rangeCheck(0x037f8000, 0x03807e00, arm7entry, 4));
}


bool importROMImage(string& container, string libraryPath, string sourcePath) {
  file image; image.open(sourcePath, file::mode::read);
  
  image.seek(0x68);
  
  uint32_t bannerOffset = image.readl(4);
  image.seek(bannerOffset);
  
  uint16_t bannerVersion     = image.readl(2);
  uint16_t bannerCrc         = image.readl(2);
  uint16_t bannerCrc2        = image.readl(2);
  uint8_t  bannerData[0xa00] = {0};
  
  image.seek(bannerOffset + 0x20);
  image.read(bannerData, bannerVersion > 1? 0x940 : 0x840);
  image.close();
  
  bool     bannerValid = bannerOffset && bannerCrc == crc16(bannerData, 0x820);
  uint8_t  bannerTextBuf[0x100+2] = {0};
  
  if(bannerValid) {
    // Not sure if this is UCS-2 or full UTF-16.
    memcpy(bannerTextBuf, bannerData + 0x320, 0x100);
  }
  lstring bannerText = string(utf8fromUcs2(bannerTextBuf)).split("\n");
  string bannerTitle = bannerText(0).trim();
  string bannerSubTitle = bannerText(1).trim();
  string bannerExtra = bannerText(2).trim();
  string defaultName = bannerTitle;
  
  if(bannerExtra)
    defaultName.append(" - ",bannerSubTitle);
  
  if(!defaultName)  // eg. if no banner
    defaultName = basename(notdir(sourcePath));
  
  string manifest = findManifest(sourcePath, defaultName, true);
  auto elem = Markup::Document(manifest);
  print("Manifest:\n",manifest,"\n");
  string name   = elem["title"].text();        // Title "The Legend of Foo"
  string folder = elem["title/file"].text();   //   eg. "Legend of Foo, The"
  string id     = elem["title/id"].text();
  
  if(!folder) folder = name;
  
  // Remove invalid characters from folder name
  folder = defaultName.replace(": - ", " - ").replace(": ", " - ");
  folder = folder.transform("<>\"\\|/?*:", "()\'------");
  
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
  
#if defined(_WIN32)
  PathMakeSystemFolder(utf16_t(container));
  
  file ini; ini.open({container, "desktop.ini"}, file::mode::write);
  ini.print("[.ShellClassInfo]\n"
            "ConfirmFileOp=0\n"
            "IconFile=banner.ico\n"
            "IconIndex=0\n"
            "InfoTip=",bannerExtra? bannerExtra : bannerSubTitle,"\n");
#endif
  if(bannerValid)
    convertIcon({container, "banner.ico"}, bannerData);
  return true;
}

}
