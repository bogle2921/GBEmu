#include "graphics.h"



struct tile tiles[H][W] = {
    [0 ... H-1] = {
        [0 ... W-1] = {
            .pixels = {
                [0 ... TH-1] = {
                    [0 ... TW-1] = { .value = 0.0 }
                }
            }
        }
    }
};


