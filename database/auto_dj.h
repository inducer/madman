/*
 * madman - a music manager
 *
 * auto_dj.h Copyright 2003 Shawn Willden <shawn@willden.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifndef HEADER_SEEN_AUTODJ_H
#define HEADER_SEEN_AUTODJ_H

// Local includes
#include "utility/base.h"

// Forward declarations to avoid #includes
class QString;
class tAutoDJPreferences;
class tSong;
class tSongSet;

class tAutoDJ
{
  public:
    tAutoDJ(const tAutoDJPreferences& prefs, const tSongSet* song_set);
    ~tAutoDJ();

    // Change the songs or preferences used by this DJ
    void setSongs(const tSongSet *song_set);
    void setPreferences(const tAutoDJPreferences& prefs);

    // Select one song, or a set of songs
    const tSong *selectSong();
    void selectSongs(tSongList& result, unsigned count);

    // Re-evaluate scores, based on updated song data.  This will go
    // away when we hook more fully into tSong.
    void reEvaluateScores() { clearScores(); }

  private:
    void addCriterion(const QString& criterion, int weight);
    float score(const tSong *song) const;

    void clearCompiledScoringRules();
    void clearScores();

    void calculateScores();
    void checkInvariant() const;

    struct tAutoDJImpl* const pimpl;
};


#endif // AutoDJ_h




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
