/*
madman - a music manager
Copyright (C) 2004  Andreas Kloeckner <ak@ixion.net>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/




#ifndef HEADER_SEEN_CRITERION_H
#define HEADER_SEEN_CRITERION_H




#include "utility/base.h"




class tSong;
class tSongSet;




// tCriterion -----------------------------------------------------------------
class tCriterion
{
  public:
    virtual ~tCriterion()
    { }
    virtual float matchDegree(const tSong *song) const = 0;

    /** Returns whether a song would emit a change notification signal
	if this criterion begins or ceases to apply.
    */
    virtual bool notifying() const
      { return true; }
};




class tInSetCriterion : public tCriterion
{
    tSongSet *SongSet;
  public:
    tInSetCriterion(tSongSet *ss);
    float matchDegree(const tSong *song) const;
};




class tOrCriterion : public tCriterion
{
    auto_ptr<tCriterion>	Operand1,Operand2;
  public:
    tOrCriterion(tCriterion *op1, tCriterion *op2)
      : Operand1(op1), Operand2(op2)
      { }
    float matchDegree(const tSong *song) const
      {
	return max(Operand1->matchDegree(song), Operand2->matchDegree(song));
      }
    bool notifying() const
      { return Operand1->notifying() && Operand2->notifying(); }
};




class tAndCriterion : public tCriterion
{
    auto_ptr<tCriterion>	Operand1,Operand2;
  public:
    tAndCriterion(tCriterion *op1, tCriterion *op2)
      : Operand1(op1), Operand2(op2)
      { }
    float matchDegree(const tSong *song) const
      {
	return min(Operand1->matchDegree(song), Operand2->matchDegree(song));
      }
    bool notifying() const
      { return Operand1->notifying() && Operand2->notifying(); }
};




class tNotCriterion : public tCriterion
{
    auto_ptr<tCriterion>	Operand;
  public:
    tNotCriterion(tCriterion *op)
      : Operand(op)
      { }
    float matchDegree(const tSong *song) const
      {
	return 1 - Operand->matchDegree(song);
      }
    bool notifying() const
      { return Operand->notifying(); }
};




auto_ptr<tCriterion> parseCriterion(QString criterion);




#endif




// EMACS-FORMAT-TAG
//
// Local Variables:
// mode: C++
// eval: (c-set-style "stroustrup")
// eval: (c-set-offset 'access-label -2)
// eval: (c-set-offset 'inclass '++)
// c-basic-offset: 2
// tab-width: 8
// End:
