#include "geo/tiles.h"

namespace valhalla {
namespace geo {

Tiles::Tiles(const AABB2& bounds, const float tilesize) {
  tilebounds_ = bounds;
  tilesize_ = tilesize;
  ncolumns_ = (int) ceil((bounds.maxx() - bounds.minx()) / tilesize_);
  nrows_ = (int) ceil((bounds.maxy() - bounds.miny()) / tilesize_);
}

Tiles::~Tiles() {
}

float Tiles::TileSize() const {
  return tilesize_;
}

AABB2 Tiles::TileBounds() const {
  return tilebounds_;
}

int Tiles::Row(const float y) const {
  // Return -1 if outside the tile system bounds
  if (y < tilebounds_.miny() || y > tilebounds_.maxy())
    return -1;

  // If equal to the max y return the largest row
  if (y == tilebounds_.maxy())
    return nrows_ - 1;
  else {
    return (int) ((y - tilebounds_.miny()) / tilesize_);
  }
}

int Tiles::Col(const float x) const {
  // Return -1 if outside the tile system bounds
  if (x < tilebounds_.minx() || x > tilebounds_.maxx())
    return -1;

  // If equal to the max x return the largest column
  if (x == tilebounds_.maxx())
    return ncolumns_ - 1;
  else {
    float col = (x - tilebounds_.minx()) / tilesize_;
    return (col >= 0.0) ? (int) col : (int) col - 1;
  }
}

int Tiles::TileId(const Point2& c) {
  return TileId(c.y(), c.x());
}

int Tiles::TileId(const float y, const float x) {
  // Return -1 if totally outside the extent.
  if (y < tilebounds_.miny() || x < tilebounds_.minx() ||
      y > tilebounds_.maxy() || x > tilebounds_.maxx())
    return -1;

  // Find the tileid by finding the latitude row and longitude column
  return (Row(y) * ncolumns_) + Col(x);
}

int Tiles::TileId(const int col, const int row) {
  return (row * ncolumns_) + col;
}

Point2 Tiles::Base(const int tileid) const {
  int row = tileid / ncolumns_;
  int col = tileid - (row * ncolumns_);
  return Point2(tilebounds_.miny() + (row * tilesize_),
                tilebounds_.minx() + (col * tilesize_));
}

AABB2 Tiles::TileBounds(const int tileid) const {
  Point2 base = Base(tileid);
  return AABB2(base.y(), base.x(), base.y() + tilesize_, base.x() + tilesize_);
}

AABB2 Tiles::TileBounds(const int col, const int row) const {
  float basey = ((float) row * tilesize_) + tilebounds_.miny();
  float basex = ((float) col * tilesize_) + tilebounds_.minx();
  return AABB2(basey, basex, basey + tilesize_, basex + tilesize_);
}

Point2 Tiles::Center(const int tileid) const {
  Point2 base = Base(tileid);
  return Point2(base.y() + tilesize_ * 0.5, base.x() + tilesize_ * 0.5);
}

int Tiles::GetRelativeTileId(const int initial_tile, const int delta_rows,
                             const int delta_cols) const {
  return initial_tile + (delta_rows * ncolumns_) + delta_cols;
}

void Tiles::TileOffsets(const int initial_tileid, const int newtileid,
                        int& delta_rows, int& delta_cols) const {
  int deltaTile = newtileid - initial_tileid;
  delta_rows = (newtileid / ncolumns_) - (initial_tileid / ncolumns_);
  delta_cols = deltaTile - (delta_rows * ncolumns_);
}

unsigned int Tiles::TileCount() const {
  float nrows = (tilebounds_.maxy() - tilebounds_.miny()) / tilesize_;
  return ncolumns_ * (int) ceil(nrows);
}

int Tiles::RightNeighbor(const int tileid) const {
  int row = tileid / ncolumns_;
  int col = tileid - (row * ncolumns_);
  return (col < ncolumns_ - 1) ? tileid + 1 : tileid - ncolumns_ + 1;
}

int Tiles::LeftNeighbor(const int tileid) const {
  int row = tileid / ncolumns_;
  int col = tileid - (row * ncolumns_);
  return (col > 0) ? tileid - 1 : tileid + ncolumns_ - 1;
}

int Tiles::TopNeighbor(const int tileid) const {
  return
      (tileid < (int) (TileCount() - ncolumns_)) ? tileid + ncolumns_ : tileid;
}

int Tiles::BottomNeighbor(const int tileid) const {
  return (tileid < ncolumns_) ? tileid : tileid - ncolumns_;
}

const std::vector<int>& Tiles::TileList(const AABB2& boundingbox,
                                        const unsigned int maxtiles) {
  // Clear lists
  checklist_.clear();
  tilelist_.clear();
  visitedtiles_.clear();

  // Get tile at the center of the boundinb box. Return -1 if the center
  // of the bounding box is not within the tiling system bounding box.
  // TODO - relax this to check edges of the bounding box?
  int tileid = TileId(boundingbox.Center());
  if (tileid == -1)
    return tilelist_;

  // Set this tile in the list and it to the list of visited tiles.
  tilelist_.push_back(tileid);
  visitedtiles_[tileid] = 1;

  // Add neighbors to the "check" list
  addNeighbors(tileid);

  // Get neighboring tiles in bounding box until NextTile returns -1
  // or the maximum number specified is reached
  while ((tileid = NextTile(boundingbox)) >= 0) {
    tilelist_.push_back(tileid);
    if (tilelist_.size() == maxtiles) {
      break;
    }
  }
  return tilelist_;
}

Tiles::Tiles()
    : tilesize_(0),
      nrows_(0),
      ncolumns_(0) {
}

void Tiles::addNeighbors(const int tileid) {
  // Make sure we check that the neighbor tile is not equal to the
  // current tile - that happens at the edge of the coverage
  int neighbor = LeftNeighbor(tileid);
  if (neighbor != tileid && !InList(neighbor)) {
    checklist_.push_back(neighbor);
    visitedtiles_[neighbor] = 1;
  }

  neighbor = RightNeighbor(tileid);
  if (neighbor != tileid && !InList(neighbor)) {
    checklist_.push_back(neighbor);
    visitedtiles_[neighbor] = 1;
  }

  neighbor = TopNeighbor(tileid);
  if (neighbor != tileid && !InList(neighbor)) {
    checklist_.push_back(neighbor);
    visitedtiles_[neighbor] = 1;
  }

  neighbor = BottomNeighbor(tileid);
  if (neighbor != tileid && !InList(neighbor)) {
    checklist_.push_back(neighbor);
    visitedtiles_[neighbor] = 1;
  }
}

int Tiles::NextTile(const AABB2& boundingbox) {
  int tileid;
  while (!checklist_.empty()) {
    // Get the element off the front of the list
    tileid = checklist_.front();
    checklist_.pop_front();

    // Check if tile is visible
    if (boundingbox.Intersects(TileBounds(tileid))) {
      // Add neighbors
      addNeighbors(tileid);
      return tileid;
    }
  }
  return -1;
}

bool Tiles::InList(const int id) {
  std::map<int, int>::iterator iter = visitedtiles_.find(id);
  return (iter != visitedtiles_.end());
}
}
}
