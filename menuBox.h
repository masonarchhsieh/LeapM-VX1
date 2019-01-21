/* Define menuBOX class */
#ifndef MENUBOX
#define MENUBOX

#include <stdlib.h>
#include <math.h>
#include <process.h>
#include <malloc.h>

// Import values/functions from voxie main file(.c)
#ifdef __cplusplus
extern "C"
{
	#include "voxiebox.h"
}
#endif

#define UNPRESSED 0
#define PRESSED 1


class MenuBox
{
private:
  int status = 0;
  int timer = 0;
	point3d p, r, d, f;

public:
  MenuBox();
  int getStatus() { return this->status; }
  bool checkHitBox(point3d pp, point3d rr, point3d dd, point3d ff, point3d indexPos);
  void nextEvent();


};

#endif MENUBOX
