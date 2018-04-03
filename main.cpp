#include <ncurses.h>
#include <iostream>
#include <string>
#include <time.h>
#include <stdlib.h>
#include <locale.h>

/*
 * TODO:
 *  - convert arrays to vectors (most importantly TileGlyph logic)
 *  - convert map, tiles to std::map ?
 *  - integrate adaptive probabilities
 *  - extract tile type defs into aux file
 *  - extract ncurses/rendering to aux file
 *  - templates?
 */

WINDOW* win;

void init();

typedef std::string glyph_t;

enum TileGlyph{
  Grass,//               = "\u2591",
  RoadEastWest,//        = "\u2501",
  RoadNorthSouth,//      = "\u2503",
  RoadNorthWest,//       = "\u251B",
  RoadNorthEast,//       = "\u2517",
  RoadSouthWest,//       = "\u2513",
  RoadSouthEast,//       = "\u250F"
  RoadNorthSouthEastWest// = "\u254B"
};

//std::string getGlyph(TileGlyph g){
glyph_t getGlyph(TileGlyph g){
  switch(g){
    case Grass          : return "\u2591";
    case RoadEastWest   : return "\u2500";
    case RoadNorthSouth : return "\u2502";
    case RoadNorthWest  : return "\u2518";
    case RoadNorthEast  : return "\u2514";
    case RoadSouthWest  : return "\u2510";
    case RoadSouthEast  : return "\u250c";
    case RoadNorthSouthEastWest: return "\u253c";
    default: return "x";
  }
}

/*
 * Tiles need to encode the following:
 * - glyph corresponding to tile type
 * - 4 direct edge constraints
 * - sphere of probabilities of other blocks (8 around)
 *   - each will be an array for each other tile type
 */

struct Tile {
  //TileGlyph blocks[3][3];
  //Constraint cons[4]; // left, top, right, bot
  //double probs[3][3]; // row major
  TileGlyph blocks[3][3]; // implicit constraints

  Tile(){};

  // Non-default ctor, pass a 3x3 matrix
  Tile(TileGlyph arg[3][3]){
    for(int i=0; i<3; i++){
      for(int j=0; j<3; j++){
        blocks[i][j] = arg[i][j];
      }
    }
  }
};

/*
 * Stores data actually on the map:
 * - tile type
 * - status (undecided/decided)
 * - probability distribution
 */
struct MapTile {
  Tile t;
  bool decided;
  double* probs;
};

namespace Tiles {
  /*  * | *
   *  - + -
   *  * | *
   */
  TileGlyph _plus[3][3] = {
    {Grass, RoadNorthSouth, Grass},
    {RoadEastWest, RoadNorthSouthEastWest, RoadEastWest},
    {Grass, RoadNorthSouth, Grass}
  };

  TileGlyph _top_left[3][3] = {
    {Grass, RoadNorthSouth, Grass},
    {RoadEastWest, RoadNorthWest, Grass},
    {Grass, Grass, Grass}
  };

  TileGlyph _top_right[3][3] = {
    {Grass, RoadNorthSouth, Grass},
    {Grass, RoadNorthEast, RoadEastWest},
    {Grass, Grass, Grass}
  };

  TileGlyph _bottom_right[3][3] = {
    {Grass, Grass, Grass},
    {Grass, RoadSouthEast, RoadEastWest},
    {Grass, RoadNorthSouth, Grass}
  };

  TileGlyph _bottom_left[3][3] = {
    {Grass, Grass, Grass},
    {RoadEastWest, RoadSouthWest, Grass},
    {Grass, RoadNorthSouth, Grass}
  };

  TileGlyph _up_down[3][3] = {
    {Grass, RoadNorthSouth, Grass},
    {Grass, RoadNorthSouth, Grass},
    {Grass, RoadNorthSouth, Grass}
  };

  TileGlyph _left_right[3][3] = {
    {Grass, Grass, Grass},
    {RoadEastWest, RoadEastWest, RoadEastWest},
    {Grass, Grass, Grass}
  };

  TileGlyph _grass[3][3] = {
    {Grass, Grass, Grass},
    {Grass, Grass, Grass},
    {Grass, Grass, Grass}
  };

  Tile types[] = {
    _plus,
    _top_left,
    _top_right,
    _bottom_right,
    _bottom_left,
    _left_right,
    _up_down
  };

  const int num_tiles = 8;

  void printAt(Tile t, int y, int x){
    for(int i=0; i<3; i++){
      for(int j=0; j<3; j++){
        //printf("%s",getGlyph(t.blocks[i][j]).c_str());
        mvwaddstr(win,y+i,x+j,getGlyph(t.blocks[i][j]).c_str());
      }
    }
  }
};

struct Map {
  MapTile** _cells;
  int _h, _w;

  Map(int h, int w){
    _h = h;
    _w = w;
    _cells = new MapTile*[h];
    for(int i=0; i<h; i++){
      _cells[i] = new MapTile[w];
    }
  }

  ~Map(){
    for(int i=0; i<_h; i++){
      delete [] _cells[i];
    }
    delete [] _cells;
  }

  MapTile* operator[](int i) const{
    return _cells[i];
  }
};

void printMap(const Map&);

void printarr(int* arr, int len){
  for(int i=0; i<len; i++){
    printf("%d ", arr[i]);
  }
  printf("\n");
}

int main(){
  srand(time(NULL));
  rand();

  init();
  Tile t;

  const int h = LINES/3;
  const int w = COLS/3;

  Map map(h,w);

  while(1){

    int matches[Tiles::num_tiles];
    int num_matches = 0;
    int rand_match;
    bool match = true;

    //map[0][0].t = Tiles::types[rand()%Tiles::num_tiles];
    
    // Iterate each block of the map, choosing a random tile
    // that matches. Currently goes ltr, ttb
    for(int i=0; i<h; i++){
      for(int j=0; j<w; j++){
        //map[i][j].t = Tiles::types[rand()%Tiles::num_tiles];
        num_matches = 0;
        for(int k=0; k<Tiles::num_tiles; k++){
          match = true;
          // Check the left && right
          if(j > 0 && Tiles::types[k].blocks[1][0] != map[i][j-1].t.blocks[1][2]){
            match = false;
          }
          // Check top and bottom
          if(i > 0 && Tiles::types[k].blocks[0][1] != map[i-1][j].t.blocks[2][1]){
            match = false;
          }
          if(match){
            matches[num_matches] = k;
            num_matches++;
          }
        }
        if(num_matches > 0){
          //mvwprintw(win, 0, 20, "Number of matches found for (%d,%d): %d\n", i, j, num_matches);
          //print2("Match indexes:\n");
          //printarr(matches,num_matches);
          rand_match = rand()%num_matches;
          //mvwprintw(win, 1, 20, "Random one: %d\n", rand_match);
          //wrefresh(win);
          //getch();
          map[i][j].t = Tiles::types[matches[rand_match]];
        } else {
          printf("wtf?\n");
        }
      }
    }

    printMap(map);

    // Wait for user-input to re-generate the map
    wgetch(win);
  }
  endwin();


  return 0;
}

void printMap(const Map& m){
  for(int i=0; i<m._h; i++){
    for(int j=0; j<m._w; j++){
      //mvaddstr(i,j,"o");
      Tiles::printAt(m[i][j].t, 3*i, 3*j);
    }
  }
  wrefresh(win);
}

void init(){
  int height, width;

  setlocale(LC_ALL, "");

  initscr();
  cbreak();
  noecho();
  curs_set(0);

  refresh();

  height = LINES;
  width = COLS;

  win = newwin(height, width, 0, 0);
  //box(win,0,0);
  wrefresh(win);

  wrefresh(stdscr);
}
