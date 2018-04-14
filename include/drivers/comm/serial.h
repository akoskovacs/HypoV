/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2018                           |
 * |                                                            |
 * | PC Serial device, handled through the generic character    |
 * | display interface                                          |
 * +------------------------------------------------------------+
*/
#ifndef SERIAL_H
#define SERIAL_H

struct CharacterDisplay;

int hv_serial_init(struct CharacterDisplay *display);

#endif // SERIAL_H