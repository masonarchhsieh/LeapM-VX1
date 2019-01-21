/*
 * =====================================================================================
 *
 *       Filename:  Leap_Voxon.cpp
 *
 *    Description:
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
  #include <stdlib.h>
  #include <math.h>
  #include <process.h>
  #include <malloc.h>
  #include "LeapC.h"
  #include "Leap_Voxon.h"

  Leap_Voxon::Leap_Voxon()
  {
    leap_init();
  }

  void Leap_Voxon::track_event()
  {

    if (frame)
    {

      if(frame->nHands==0x00)
      {
        if (this->isGrabbed) { this->isGrabbed = false; }
        if (this->isPinched) { this->isPinched = false; }
        if (this->hasHand) { this->hasHand = false; }
        if (this->isSwiped) { this->isSwiped = false; }
        if (this->eventType != nullptr) { this->eventType = nullptr; }
        if (this->numHands != 0) { this->numHands = 0; }
      }
      else
      {
        if(!this->hasHand) { this->hasHand = true; }

        // it can only detect 4 hands at the same time
        for (int i=min(frame->nHands,4)-1;i>=0;i--)
        {
          /* detect hands and draw fingers */
          hand = &frame->pHands[i];
          palm = &hand->palm;
          if (hand->type == eLeapHandType_Left)
          {
            handl = hand;
            palml = &handl->palm;
          }
          else if (hand->type == eLeapHandType_Right)
          {
            handr = hand;
            palmr = &handr->palm;
          }
        }


        // Decide which hand is primary hand
        // if there're 2 hands, always select the one detected time longer
        if (frame->nHands == 0x01) { track_this_hand(hand,palm); this->numHands = 1;}
        else { track_this_hand(hand,palm); this->numHands = 2; }
      }
  	  Sleep(10);
    }
  }

	void Leap_Voxon::track_this_hand(LEAP_HAND *this_hand, LEAP_PALM *this_palm)
  {
    // Testing for Grab/Pinch function/
    if (this_hand->grab_strength >= grabbed_strength)
    {
      if (!this->isGrabbed) { isGrabbed = true;}
    }
    else
    {
      if (this->isGrabbed) { isGrabbed = false; }
    }

    if (this_hand->pinch_strength >= pinched_strength && this_hand->pinch_distance <= 15)
    {
      if (!this->isPinched) { isPinched = true; }
    }
    else
    {
      if (this->isPinched) { isPinched = false; }
    }

    // For tracking swipe gesture
    if (this->swipemode) { track_swipe(this_palm); }
    // End for testing

    // Change the eventType value
    updateEvent();
  }

  void Leap_Voxon::updateEvent()
  {
    prev_eventType = eventType;
    if(isSwiped && isGrabbed) { eventType = "Swipe+Grab"; }
    else if(isSwiped && isPinched) { eventType = "Swipe+Pinch"; }
    else if(isSwiped) { eventType = "Swipe"; }
    else if(isGrabbed) { eventType = "Grab"; }
    else if(isPinched) { eventType = "Pinch"; }
    else { if(eventType != nullptr) { eventType = nullptr; }}

    if ( prev_eventType!=eventType) { if (!this->eventLast) this->eventLast = true; }
    else { if (this->eventLast) { this->eventLast = false; }}
  }

  void Leap_Voxon::set_swipemode(bool new_swipemode)
  {
    if (this->swipemode != new_swipemode) { this->swipemode = new_swipemode; }
  }

  // Track swipe gesture:
  // SwipeType: Up:0, Down:1, Left:2, Right:3, Forward:4, Backward:5
  // Based on web page: https://developer.leapmotion.com/documentation/v4/concepts.html
  void Leap_Voxon::track_swipe(LEAP_PALM *this_palm)
  {
    if (fabs(this_palm->velocity.x) >= swiped_vel ||
      fabs(this_palm->velocity.y) >= swiped_vel ||
      fabs(this_palm->velocity.z) >= swiped_vel)
    {
      if(!this->isSwiped) { isSwiped = true; }

      // Decide swipe type
      // SwipeType: Up:0, Down:1, Left:2, Right:3, Forward:4, Backward:5
      if (fabs(this_palm->velocity.x) >= swiped_vel &&
        fabs(this_palm->velocity.y) <= swiped_vel &&
        fabs(this_palm->velocity.z) <= swiped_vel)
      {
        if (this->palm->velocity.x >= swiped_vel) { swipeType = 3; }
        else { swipeType = 2; }
      }

      else if (fabs(this_palm->velocity.x) <= swiped_vel &&
        fabs(this_palm->velocity.y) >= swiped_vel &&
        fabs(this_palm->velocity.z) <= swiped_vel)
      {
        if (this->palm->velocity.y >= swiped_vel) { swipeType = 0; }
        else { swipeType = 1; }
      }

      else if (fabs(this_palm->velocity.x) <= swiped_vel &&
        fabs(this_palm->velocity.y) <= swiped_vel &&
        fabs(this_palm->velocity.z) >= swiped_vel)
      {
        if (this->palm->velocity.z >= swiped_vel) { swipeType = 5; }
        else { swipeType = 4; }
      }

      else
      {
        if (swipeType != -1) { swipeType = -1; }
      }

    }
    else
    {
      if(this->isSwiped) { isSwiped = false; }
      if (swipeType != -1) { swipeType = -1; }
    }
  }


  Leap_Voxon::~Leap_Voxon() { leap_uninit(); }
