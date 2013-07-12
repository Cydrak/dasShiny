#ifndef UTILTIY_HPP
#define UTILITY_HPP

#ifndef NDS_HPP
#include <nall/base64.hpp>
#include <nall/file.hpp>
#include <nall/filemap.hpp>
#include <nall/directory.hpp>
#include <nall/map.hpp>
#include <nall/set.hpp>
#include <nall/stdint.hpp>
#include <nall/stream.hpp>
#include <nall/string.hpp>
#include <nall/vector.hpp>

namespace NintendoDS {
#endif

uint16_t crc16(uint8_t* data, unsigned size, uint16_t initial = ~0);
nall::string sha1(uint8_t *data, uint64_t size);

bool validateHeader(const nall::stream& fp);

void loadDatabase(nall::string markup);

// Look manifests up in the database based on game serial in the ROM image.
// If an exact match isn't found, it will return heuristic results.
//
// - name - A default title to be used if an exact match isn't found.
// - hash - If true, match based on any available hash, too.
// 
nall::string findManifest(nall::string imagePath,
  nall::string name = nall::string(), bool hash = false);

nall::string findManifest(uint8_t *data, unsigned size,
  nall::string name = nall::string(), bool hash = false);

// Import a ROM image into a container. This results in a game folder,
// <libraryPath>/<title>.nds/, containing manifest.bml and a copy of the image.
bool importROMImage(nall::string& container,
  nall::string libraryPath, nall::string sourcePath);

#ifndef NDS_HPP
}
#endif

#endif
