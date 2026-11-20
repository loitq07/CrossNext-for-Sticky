#pragma once

#include <cstdint>

// The islands, taken from the squares of "Island Layout.pdf", a collection of official and
// fan-made tile layouts: the official ones first, Forbidden Island leading, then the fan-made ones
// from A to Z. Each is its rows from the top, '#' for a tile and '.' for sea. The three the PDF
// leaves unnamed have names made up for them: LADDER CAY, SERPENT SHOALS and SPYGLASS ISLE.
//
// Most have 24 tiles. Four don't in the PDF, and are kept as it draws them: Coral Reef and Palm
// Springs have 25, Hangman's Harbour 26, and Serpent Shoals 22.
//
// The save holds a position in this list, so only add to its end.
namespace islandmaps {

struct Island {
  const char* name;
  uint8_t width;
  uint8_t height;
  const char* cells;  // width * height, row by row
};

constexpr int kMaxWidth = 11;
constexpr int kMaxHeight = 10;

constexpr Island kIslands[] = {
    {"FORBIDDEN ISLAND", 6, 6,  // 24 tiles
     "..##.."
     ".####."
     "######"
     "######"
     ".####."
     "..##.."},
    {"ARCH OF FATE", 9, 5,  // 24 tiles
     "...###..."
     "..#####.."
     ".###.###."
     "###...###"
     "##.....##"},
    {"ATOLL OF DECISIONS", 7, 7,  // 24 tiles
     "..###.."
     ".##.##."
     "##...##"
     "#.....#"
     "##...##"
     ".##.##."
     "..###.."},
    {"BAY OF GULLS", 8, 6,  // 24 tiles
     "...##..."
     "...##..."
     "#..##..#"
     "########"
     ".######."
     "...##..."},
    {"BONE ISLAND", 8, 4,  // 24 tiles
     ".##..##."
     "########"
     "########"
     ".##..##."},
    {"BRIDGE OF HORRORS", 10, 3,  // 24 tiles
     "###...####"
     "##########"
     "####...###"},
    {"CORAL REEF", 7, 6,  // 25 tiles
     ".##...."
     "#####.."
     "######."
     ".######"
     "...####"
     "....##."},
    {"ISLAND OF DEATH", 7, 6,  // 24 tiles
     "..###.."
     "#######"
     ".#####."
     "..###.."
     "..###.."
     "..###.."},
    {"ISLAND OF SHADOWS", 6, 4,  // 24 tiles
     "######"
     "######"
     "######"
     "######"},
    {"SKULL ISLAND", 6, 5,  // 24 tiles
     "######"
     "#.##.#"
     "######"
     ".####."
     ".####."},
    {"VOLCANO ISLAND", 5, 5,  // 24 tiles
     "#####"
     "#####"
     "##.##"
     "#####"
     "#####"},
    {"ATOLL OF APPREHENSION", 6, 7,  // 24 tiles
     "####.."
     "#....."
     "###..."
     "####.."
     "#####."
     ".#####"
     "...##."},
    {"BALEFUL BLUFFS", 6, 5,  // 24 tiles
     ".####."
     "##.###"
     "######"
     "####.#"
     ".####."},
    {"BARBARY COAST", 6, 5,  // 24 tiles
     ".#####"
     "##.##."
     "######"
     ".##.##"
     "#####."},
    {"BERMUDA TRIANGLE", 9, 5,  // 24 tiles
     "....#...."
     "...###..."
     "..#####.."
     ".###.###."
     "#########"},
    {"BLUNDERBUSS BAY", 8, 5,  // 24 tiles
     "...#...#"
     "########"
     "########"
     "###....#"
     "##......"},
    {"BREAKERS BRIDGE", 8, 5,  // 24 tiles
     "..#...#."
     ".###.###"
     "########"
     ".###.###"
     "..#...#."},
    {"CHALICE ISLAND", 7, 5,  // 24 tiles
     "##...##"
     ".#####."
     "..###.."
     ".#####."
     "#######"},
    {"DAVY JONES' LOCKER", 9, 6,  // 24 tiles
     "....#...."
     ".####...."
     "###.####."
     ".####.###"
     "....####."
     "....#...."},
    {"DEEP SIX", 6, 6,  // 24 tiles
     ".####."
     ".####."
     ".####."
     "######"
     ".####."
     "..##.."},
    {"DESCENT", 4, 10,  // 24 tiles
     ".##."
     ".##."
     ".##."
     ".##."
     ".##."
     ".##."
     ".##."
     ".##."
     "####"
     "####"},
    {"DEVIL'S BREAKER", 6, 7,  // 24 tiles
     "..###."
     "..###."
     "..##.."
     "...##."
     "..####"
     ".#####"
     "#####."},
    {"DONKEY KONG ISLAND", 7, 4,  // 24 tiles
     "######."
     ".######"
     "######."
     ".######"},
    {"DRAGON'S HAVEN", 7, 5,  // 24 tiles
     "..##.#."
     ".#####."
     "######."
     "#######"
     "#..##.."},
    {"FATMAN ISLAND", 9, 6,  // 24 tiles
     "....#...."
     "...###..."
     "#########"
     "..#####.."
     "..##.##.."
     "..#...#.."},
    {"FJORD OF DESOLATION", 8, 9,  // 24 tiles
     "##......"
     ".##....."
     "..##...."
     "..####.."
     "....####"
     "..####.."
     "..##...."
     ".##....."
     "##......"},
    {"FOR-BIDDEN", 8, 5,  // 24 tiles
     "#######."
     "#...#.#."
     "##..####"
     "#...#..#"
     "#...####"},
    {"HANGMAN'S HARBOUR", 7, 8,  // 26 tiles
     "..##..."
     ".####.."
     "######."
     "#...##."
     "#...##."
     "....##."
     "....##."
     "...####"},
    {"HARPOON HIDEOUT", 6, 6,  // 24 tiles
     "####.."
     "###..."
     "####.#"
     "#.####"
     "...###"
     "..####"},
    {"ISLAND OF LIBERTY", 7, 7,  // 24 tiles
     "####..."
     ".####.#"
     ".######"
     ".####.."
     ".##...."
     ".#....."
     ".##...."},
    {"ISLAND OF THE EVIL EYE", 8, 5,  // 24 tiles
     "#......#"
     "##....##"
     "##....##"
     "###..###"
     "########"},
    {"ISLE OF BLISTERS", 6, 6,  // 24 tiles
     "..####"
     "...###"
     "...###"
     "..####"
     ".#####"
     "###.##"},
    {"ISLE OF JUSTICE", 11, 6,  // 24 tiles
     ".#########."
     ".#...#...#."
     "###..#..###"
     ".....#....."
     ".....#....."
     "....###...."},
    {"LADDER CAY", 5, 6,  // 24 tiles
     "#####"
     ".###."
     "#####"
     ".###."
     "#####"
     ".###."},
    {"LINCOLN ISLE", 8, 5,  // 24 tiles
     "..####.."
     "..####.."
     "..####.."
     "..####.."
     "########"},
    {"LONG ISLAND", 8, 3,  // 24 tiles
     "########"
     "########"
     "########"},
    {"LOST ISLAND", 6, 6,  // 24 tiles
     "....#."
     "#####."
     ".####."
     "######"
     ".#####"
     "..#.##"},
    {"MUTINY TOWERS", 7, 6,  // 24 tiles
     "#.....#"
     "#.....#"
     "#.....#"
     "##...##"
     "#######"
     "#######"},
    {"OCTOPUS BAY", 8, 5,  // 24 tiles
     "..#.#.#."
     "#######."
     "..####.."
     ".#######"
     ".#.#.#.."},
    {"PALM SPRINGS", 7, 7,  // 25 tiles
     "..###.."
     ".#####."
     ".#.#.#."
     "...#..."
     "...#..."
     ".#####."
     "#######"},
    {"PARLEY PENINSULA", 6, 6,  // 24 tiles
     "###..."
     "###..."
     "######"
     "..####"
     "..####"
     "..####"},
    {"PEAKS OF PERIL", 7, 6,  // 24 tiles
     "#.#.#.#"
     "#######"
     ".####.."
     "..####."
     "..###.."
     "..##..."},
    {"PERILOUS PEAKS", 6, 6,  // 24 tiles
     "#.#.#."
     "#.#.#."
     "######"
     "..####"
     ".####."
     "..####"},
    {"PHANTOM ROCK", 8, 4,  // 24 tiles
     ".######."
     "##..####"
     "####..##"
     ".######."},
    {"RAINBOW BANK", 10, 5,  // 24 tiles
     "....##...."
     "...####..."
     "..######.."
     ".###..###."
     "###....###"},
    {"ROASTED REFUGE", 6, 7,  // 24 tiles
     ".#..#."
     "##..##"
     ".####."
     ".####."
     ".####."
     "##..##"
     ".#..#."},
    {"SABRE ISLAND", 5, 10,  // 24 tiles
     "...#."
     "..##."
     "..##."
     "..##."
     "..##."
     "..##."
     "#####"
     "#.##."
     ".###."
     "..##."},
    {"SERPENT SHOALS", 9, 5,  // 22 tiles
     ".###....."
     "..####..."
     "####.####"
     "...####.."
     ".....###."},
    {"SHARK ISLAND", 9, 5,  // 24 tiles
     "....#...."
     "##..##..."
     ".########"
     ".#######."
     "##..#.#.."},
    {"SHIPWRECK BAY", 8, 4,  // 24 tiles
     "########"
     "##.##.##"
     ".##..##."
     ".######."},
    {"SPYGLASS ISLE", 7, 6,  // 24 tiles
     ".######"
     "#..#..#"
     "#..#..#"
     "#..#..#"
     "#..#..#"
     "######."},
    {"TREASURE ISLAND", 7, 6,  // 24 tiles
     "##...##"
     ".#####."
     "..###.."
     "..###.."
     ".#####."
     "##...##"},
    {"WALK THE PLANK", 9, 6,  // 24 tiles
     "####....."
     ".####...."
     "..####..."
     "...####.."
     "....####."
     ".....####"},
    {"WALK THE PLANK 2", 9, 5,  // 24 tiles
     "...####.."
     "..#####.."
     ".####.#.."
     "####..###"
     "......###"},
    {"X MARKS THE SPOT", 8, 7,  // 24 tiles
     "#......#"
     "##....##"
     ".##..##."
     "..####.."
     ".##..##."
     "##....##"
     "#......#"},
};

constexpr int kIslandCount = sizeof(kIslands) / sizeof(kIslands[0]);

inline bool isLand(const Island& island, int column, int row) {
  return island.cells[row * island.width + column] == '#';
}

}  // namespace islandmaps
