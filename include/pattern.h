/**
 * @file pattern.h
 * @author Dimitrije Dobrota
 * @date 19 June 2022
 * @brief Pattern interface and types
 */

#ifndef PATTERN_H
#define PATTERN_H

#include "stdlib.h"

/**
 * @brief Structure representing one cell pattern to be used in the help menu
 */
typedef struct pattern_T {
  char *cells;  ///< string where 1 represents a living cell and 0 a dead one,
                /// space is used to detonate a new row
  char *name;   ///< name of the pattern
  int   height; ///< pattern height , used for cache, no need to fill manually
  int   width;  ///< pattern width , used for cache, no need to fill manually
} * pattern_T;

/// Game title using cells
struct pattern_T title = {
    "011111000000000000000000000000000000000000000000010000000000000000000000 "
    "100000100011000100001011111100000111100111111000010000000101111110111111 "
    "100000000100100110011010000000001000010100000000010000000101000000100000 "
    "100111101000010101101011111000001000010111110000010000000101111100111110 "
    "100000101111110100001010000000001000010100000000010000000101000000100000 "
    "100000101000010100001010000000001000010100000000010000000101000000100000 "
    "011111001000010100001011111100000111100100000000011111110101000000111111 ",
    "Title",
    0,
    0,
};

/// Game keybindings
struct pattern_T games[] = {
    {      "",                     "Game", 0, 0},
    {     "p",               "play/pause", 0, 0},
    {  "wasd",              "move cursor", 0, 0},
    {"arrows",              "move screen", 0, 0},
    {     "q",                     "quit", 0, 0},
    { "space",              "toggle cell", 0, 0},
    {     "v",            "visual select", 0, 0},
    {     "-", "decrease generation step", 0, 0},
    {     "+", "increase generation step", 0, 0},
    {     "[",         "decrease dt step", 0, 0},
    {     "]",         "increase dt step", 0, 0},
    {      "",                         "", 0, 0},
    {      "",            "Visual select", 0, 0},
    { "enter",                     "save", 0, 0},
    {     "x",                   "delete", 0, 0},
    {     "t",                   "toggle", 0, 0},
    {     "q",                   "cancel", 0, 0},
};

/// Still cell patterns
struct pattern_T stills[] = {
    {                  "11 11",      "block", 0, 0},
    {              "1011 1101",      "Snake", 0, 0},
    {            "010 101 010",        "Tub", 0, 0},
    {            "110 101 010",       "Boat", 0, 0},
    {         "0110 1001 0110",    "Beehive", 0, 0},
    {         "1100 1001 0011",    "Carrier", 0, 0},
    {      "11000 10011 01101", "Shillelagh", 0, 0},
    {     "110 1001 1001 0110",       "Pond", 0, 0},
    {    "0010 0101 1010 0100",      "Barge", 0, 0},
    {    "0010 0101 1010 1100",  "Long boat", 0, 0},
    {    "0110 1001 0101 0010",       "Loaf", 0, 0},
    {"00100 01010 01010 11011",        "Hat", 0, 0},
};

/// Oscillator cell patterns
struct pattern_T oscillators[] = {
    {                                   "0111 1110",           "Toad", 0, 0},
    {                                 "010 010 010",        "Blinker", 0, 0},
    {                         "0010 1010 0101 0100",          "Clock", 0, 0},
    {                         "0011 0001 1000 1100",         "Beacon", 0, 0},
    {            "0010000100 1101111011 0010000100", "Pentadecathlon", 0, 0},
    {   "110000 110100 000010 010000 001011 000011",   "Figure eight", 0, 0},
    {"11000011 10100101 00100100 10100101 11000011",     "Spark coil", 0, 0},
};

/// Spaceship cell patterns
struct pattern_T spaceships[] = {
    {                            "010 001 111",       "Glider", 0},
    {                "01001 10000 10001 11110",  "Lightweight", 0},
    {     "000100 010001 100000 100001 111110", "Mediumweight", 0},
    {"0001100 0100001 1000000 1000001 1111110",  "Heavyweight", 0},
};

/// Methuselah cell patterns
struct pattern_T methuselahs[] = {
    {               "111 101 101",         "Piheptomino", 0},
    {            "1011 1110 0100",         "B-Heptomino", 0},
    {         "11001 10001 10011", "Glider by the dozen", 0},
    {       "111 000 010 010 010",         "Thunderbird", 0},
    {   "0100000 0001000 1100111",               "Acorn", 0},
    {"00000010 11000000 01000111",             "Diehard", 0},
};

/**
 * @brief A structure representing a group of pattern_T of a certain category
 */
typedef struct pattern_group_T {
  char             *name;    ///< name of the group
  struct pattern_T *pattern; ///< array of pattern_T that comprise a group
  int               size;    ///< number of elements in a group
} pattern_group_T;

/// Pattern group containing all of the patterns for the help menu
struct pattern_group_T pattern_groups[] = {
    {"Key bindings",       games,       sizeof(games) / sizeof(struct pattern_T)},
    {       "Still",      stills,      sizeof(stills) / sizeof(struct pattern_T)},
    {  "Oscillator", oscillators, sizeof(oscillators) / sizeof(struct pattern_T)},
    {  "Spaceships",  spaceships,  sizeof(spaceships) / sizeof(struct pattern_T)},
    {  "Methuselah", methuselahs, sizeof(methuselahs) / sizeof(struct pattern_T)},
};

/// Size of  pattern_groups
int pattern_groups_s = sizeof(pattern_groups) / sizeof(pattern_group_T);

/// Get the pattern height
int pattern_height(pattern_T pattern) {
  int count = 0;
  for (char *p = pattern->cells; *p != '\0'; p++)
    if (*p == ' ')
      count++;

  return count;
}

/// Get the pattern width
int pattern_width(pattern_T pattern) {
  int count = 0;
  for (char *p = pattern->cells; *p != ' ' && *p != '\0'; p++)
    count++;

  return count;
}

#endif
