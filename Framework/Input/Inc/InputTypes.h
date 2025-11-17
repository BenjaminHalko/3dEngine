#pragma once

namespace Engine::Input
{
enum class KeyCode : uint32_t
{
    // Function keys (GLFW key codes)
    ESCAPE = 256,
    F1 = 290,
    F2 = 291,
    F3 = 292,
    F4 = 293,
    F5 = 294,
    F6 = 295,
    F7 = 296,
    F8 = 297,
    F9 = 298,
    F10 = 299,
    F11 = 300,
    F12 = 301,

    // Numbers
    GRAVE = 96,
    ONE = '1',
    TWO = '2',
    THREE = '3',
    FOUR = '4',
    FIVE = '5',
    SIX = '6',
    SEVEN = '7',
    EIGHT = '8',
    NINE = '9',
    ZERO = '0',
    MINUS = '-',
    EQUALS = '=',
    BACKSPACE = 259,

    // Main keyboard
    TAB = 258,
    Q = 'Q',
    W = 'W',
    E = 'E',
    R = 'R',
    T = 'T',
    Y = 'Y',
    U = 'U',
    I = 'I',
    O = 'O',
    P = 'P',
    LBRACKET = '[',
    RBRACKET = ']',
    BACKSLASH = '\\',

    A = 'A',
    S = 'S',
    D = 'D',
    F = 'F',
    G = 'G',
    H = 'H',
    J = 'J',
    K = 'K',
    L = 'L',
    SEMICOLON = ';',
    APOSTROPHE = '\'',
    RETURN = 257,

    LSHIFT = 340,
    Z = 'Z',
    X = 'X',
    C = 'C',
    V = 'V',
    B = 'B',
    N = 'N',
    M = 'M',
    COMMA = ',',
    PERIOD = '.',
    SLASH = '/',
    RSHIFT = 344,

    LCONTROL = 341,
    LALT = 342,
    SPACE = ' ',
    RALT = 346,
    RCONTROL = 345,

    // Arrows
    UP = 265,
    DOWN = 264,
    LEFT = 263,
    RIGHT = 262,

    // Numpad
    NUMPAD0 = 320,
    NUMPAD1 = 321,
    NUMPAD2 = 322,
    NUMPAD3 = 323,
    NUMPAD4 = 324,
    NUMPAD5 = 325,
    NUMPAD6 = 326,
    NUMPAD7 = 327,
    NUMPAD8 = 328,
    NUMPAD9 = 329,

    NUMPADENTER = 335,
};

enum class MouseButton : uint32_t
{
    LBUTTON = 0,
    RBUTTON = 1,
    MBUTTON = 2,
};
} // namespace Engine::Input