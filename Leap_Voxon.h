/*
 * =====================================================================================
 *
 *       Filename:  Leap_Voxon.h
 *
 *    Description:  Serving as a wrapper API that defines different types of gesture
 *
 *        Version:  1.0
 *        Created:  02/07/2018 01:28:52 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Yi-Ting, Hsieh (a1691807), a1691807@student.adelaide.edu.au
 *   Organization:
 *
 * =====================================================================================
 */
/* Define Leap class */
#ifndef LEAP_VOXON
#define LEAP_VOXON

#include <stdlib.h>
#include <math.h>
#include <process.h>
#include <malloc.h>

/* Load Leap Motion library */
#pragma comment(lib,"c:/LeapSDK/lib/x64/LeapC.lib")
#include "LeapC.h"

// Import values/functions from voxie main file(.c)
#ifdef __cplusplus
extern "C"
{
	#include "voxiebox.h"
}
#endif

// Import function from voxiedemo.c for using Leap Service
extern LEAP_CONNECTION leap_con;
extern LEAP_TRACKING_EVENT leap_frame[2]; //FIFO for thread safety
extern int leap_framecnt, leap_iscon, leap_isrun;
extern void leap_thread (void *_);
extern void leap_uninit (void);
extern LEAP_CONNECTION *leap_init (void);
extern LEAP_TRACKING_EVENT *leap_getframe (void);

class Leap_Voxon
{
private:
	// For Voxiebox

	// For leap motion
	LEAP_TRACKING_EVENT *frame;
	LEAP_HAND *hand, *handr, *handl;
	LEAP_PALM *palm, *palmr, *palml;

	char *eventType = nullptr;
	char *prev_eventType = nullptr;
  bool eventLast = false;		// compare current event with previous Event
	int swipeType = -1;

	// enable feature
	bool swipemode = false;

	// private gesture values for current frame
	bool isGrabbed = false;
	bool isPinched = false;
	bool isSwiped = false;
	bool leftPinch = false;
	bool rightPinch = false;

	// values for corresponding gestures
	float LeapScale = .6;		// decide the size of hands on voxon
	float grabbed_strength = .95;
	float pinched_strength = .9;
	float swiped_vel = 1000.;	//undecided

	// private track event: grab, pinch and swipe
	void track_swipe(LEAP_PALM *this_palm);
	void updateEvent();
	void track_this_hand(LEAP_HAND *this_hand, LEAP_PALM *this_palm);

	bool verbose = true;
	int rendmode = 0;
	bool hasHand = false;		// boolean value that stands for whether there is at least a hand on
	int numHands = 0;

public:
  Leap_Voxon();				// call leap_init() to intialise Leap service
	// set up value
	void set_verbose(bool new_verbose) { this->verbose = new_verbose; }				// setting verbose for debugging
	void set_rendmode(int new_rendmode) { this->rendmode = new_rendmode; }			// change rendmode
	void set_handScale(float new_LeapScale) { this->LeapScale = new_LeapScale; }	// setting hand size on canvas
  void set_swipemode(bool new_swipemode);											// enable/disable tracking swipe gesture

	// For tracking events
	void getFrame() { frame = leap_getframe(); }		// will call the leap_getframe();
	void track_event();

	// Return values
	bool hasHandOnCanvas() { return this->hasHand; }
	int getRendmode() { return this->rendmode; }
	char *getEventType() { return this->eventType; } 	// return event types
	char *getPrevEventType() { return this->prev_eventType; }
	bool ifEventFinished() { return this->eventLast; }	// return the eventLast
	int getSwipeType() { return this->swipeType; }		// return swipeType
                                                    // SwipeType: Up:0, Down:1, Left:2, Right:3, Forward:4, Backward:5

	// For drawing on voxiebox
	int getNumHands() { return this->numHands; }
  LEAP_HAND* getHand() { return this->hand; }				// return the detected hands
	LEAP_HAND* getRightHand() { return this->handr; }
	LEAP_HAND* getLeftHand() { return this->handl; }
	float getLeapScale() { return this->LeapScale; }	// return the Scale factor (hand size) on canvas

  ~Leap_Voxon();                                 	// call leap_uninit() to uninstall the device

};

#endif LEAP_VOXON
