/*
 * madman - a music manager
 *
 * auto_dj.cpp 
 * Copyright 2003 Shawn Willden <shawn@willden.org>
 * Copyright 2003 Andreas Kloeckner <ak@ixion.net>
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

// Standard C includes
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <float.h>

// Standard C++ includes
#include <algorithm>
#include <iomanip>

// Local includes
#include "auto_dj.h"
#include "database/criterion.h"
#include "song_set.h"
#include "utility/mt_random.h"
#include "utility/prefs.h"
#include "utility/refcnt_ptr.h"


namespace
{
  class tCompiledScoringRule
  {
    public:
      refcnt_ptr<tCriterion> Criterion;
      int                    ScoreAdjustment;
  };

  typedef std::vector<tCompiledScoringRule> tCompiledScoringRuleList;
}

// tAutoDJ --------------------------------------------------------------------
class tAutoDJImpl
{
  public:
    tAutoDJImpl() 
      : TotalScoreSum(-1) 
      { 
	RNG.sgenrand(time(NULL));
      }

    auto_ptr<tSongSet>     SongSet;

    tCompiledScoringRuleList CompiledScoringRules;

    tConstSongList	   Songs;
    std::vector<float>     SongScores;

    /** ScoreSums[i] records the score sum through the i * IndexInterval'th
     * song. e.g., if IndexInterval is three:
     *
     * <pre>
     *    5 ---> ScoreSums[ 0 ] == 5
     *    7
     *    3
     *    3 ---> ScoreSums[ 1 ] == 18
     *    2
     *    4
     *    6 ---> ScoreSums[ 2 ] == 30
     * </pre>
     */
    std::vector<double>    ScoreSums;
    double                 TotalScoreSum;
    mutable mtRand	   RNG;
};

tAutoDJ::tAutoDJ(const tAutoDJPreferences& prefs, tSongSet *song_set)
: pimpl(new tAutoDJImpl)
{
  setPreferences(prefs);
  setSongSet(song_set);
}

tAutoDJ::~tAutoDJ()
{
  clearScores();
  delete pimpl;
}

void tAutoDJ::setSongSet(tSongSet *song_set)
{
  auto_ptr<tSongSet> ssautoptr(song_set);
  pimpl->SongSet = ssautoptr;

  clearScores();
}

void tAutoDJ::setPreferences(const tAutoDJPreferences& prefs)
{
  clearCompiledScoringRules();

  FOREACH_CONST(first, prefs.ScoringRuleList, tAutoDJScoringRuleList)
    addCriterion(first->Criterion, first->ScoreAdjustment);
}

void tAutoDJ::clearScores()
{
  pimpl->TotalScoreSum = -1;
  pimpl->Songs.clear();
  pimpl->SongScores.clear();
  pimpl->ScoreSums.clear();
}

void tAutoDJ::clearCompiledScoringRules()
{
  pimpl->CompiledScoringRules.clear();

  clearScores();
}

void tAutoDJ::selectSongs(tSongList &result, unsigned count)
{
  try
  {
    while (count-- > 0)
    {
      const tSong* song = selectSong();
      if (!song)
        break;
      
      result.push_back(const_cast<tSong*>(song));
    }
  }
  catch (tRuntimeError& t)
  {
    // Got an error?  If we have some songs, take them.
    if (result.size() > 0)
      return;
    else
      throw;
  }
}

void tAutoDJ::addCriterion(const QString &criterion, int score_adj)
{
  tCompiledScoringRule adj;
  adj.Criterion       = parseCriterion(criterion);
  adj.ScoreAdjustment = score_adj;
  pimpl->CompiledScoringRules.push_back(adj);
}

float tAutoDJ::score(const tSong *song) const
{
  float score = 0;

  FOREACH_CONST(iter, pimpl->CompiledScoringRules, tCompiledScoringRuleList)
  {
    float match_degree = iter->Criterion->matchDegree(song);
    if (match_degree >= .5 && 
	 iter->ScoreAdjustment == tAutoDJScoringRule::ScorePlayNever)
      return 0;

    score += match_degree * iter->ScoreAdjustment;
  }

  return score;
}

inline void tAutoDJ::checkInvariant() const 
{
  assert(pimpl->Songs.size() == pimpl->SongScores.size());
}

// Index every IndexInterval'th song.  The ideal value of
// IndexInterval should be determined somehow, it's a tradeoff between
// search efficiency (since to find any song not in the index we have
// to search through at most IndexInterval songs) and update
// efficiency (since when we change a song score we have to update
// every succeeding indexed song).  We'll use a power of two.
#ifndef NDEBUG
static const int IndexInterval = 5;
#else
static const int IndexInterval = 128;
#endif

void tAutoDJ::calculateScores()
{
  checkInvariant();

  // We have at least attempted to calculate some ratings.
  pimpl->TotalScoreSum = 0;

  if (pimpl->SongSet.get() == 0 ||
      pimpl->CompiledScoringRules.size() == 0)
    // We have no song set, or we have no adjustments.  Give up.
    return;

  tConstSongList list;
  pimpl->SongSet->render(list);

  if (list.size() == 0)
    // No songs.  Give up.
    return;

  // Iterate through all songs, calculating ratings for each.
  FOREACH(iter, list, tConstSongList)
  {
    const tSong *song = *iter;
    float song_score = score(song);

    // Ignore songs with a non-positive score.
    if (song_score > 0)
    {
      pimpl->Songs.push_back(song);
      pimpl->SongScores.push_back(song_score);
      pimpl->TotalScoreSum += song_score;

      int last_song_index = pimpl->Songs.size() - 1;

      // Note that the first time through the loop, last_song_index == 0
      // so the score of the very first song in the list is always  in
      // ScoreSums.
      if (last_song_index % IndexInterval == 0)
	pimpl->ScoreSums.push_back(pimpl->TotalScoreSum);
    }

  }

#ifndef NDEBUG
  {
    // This is horribly inefficient, but it's just for debugging, so who cares.
    unsigned dropouts = list.size() - pimpl->Songs.size();

    float max_score = - FLT_MAX;
    float min_score = FLT_MAX;

    FOREACH(first, pimpl->SongScores, vector<float>)
    {
      if (max_score < *first)
	max_score = *first;
      if (min_score > *first)
	min_score = *first;
    }

    cout 
      << "autodj score calculation statistics" << endl
      << "-----------------------------------" << endl
      << "songs eliminated from selection list: " 
      << dropouts << " out of " << list.size() << endl
      << "maximum score: " << max_score << endl
      << "minimum score: " << min_score << endl
      << "average score: " << pimpl->TotalScoreSum / (list.size() - dropouts) << endl;

    int min_s_int = int(min_score), max_s_int = int(max_score);
    for (int score = min_s_int; score <= max_s_int; score++)
    {
      unsigned count = 0;
      tUniqueId example_id = 0;

      for (unsigned i = 0; i < pimpl->Songs.size(); i++)
      {
	if (int(pimpl->SongScores[ i ]) == score)
	{
	  ++count;
	  example_id = pimpl->Songs[ i ]->uniqueId();
	}
      }
      double fraction = double(count) / (list.size() - dropouts);
      cout 
	<< "score " << setw(2) << score << ": " 
	<< setw(5) << count << " " 
	<< "(e.g. id " << setw(5) << example_id << ") " 
	<< string(int(fraction * 70), '*') << endl;
    }
  }
#endif // not NDEBUG

  checkInvariant();
}

const tSong* tAutoDJ::selectSong()
{
  checkInvariant();

  if (pimpl->TotalScoreSum == -1)
    calculateScores();

  if (pimpl->TotalScoreSum == 0)
    throw tRuntimeError("AutoDJ: All songs have zero or negative score. "
			 "Unable to perform song selection.");

  // Pick a random offset into the ratings
  double which = pimpl->RNG.gendrand() * pimpl->TotalScoreSum;

  // Figure out which song it corresponds to

  std::vector<double>::iterator i;
  i = std::lower_bound(pimpl->ScoreSums.begin(), pimpl->ScoreSums.end(), which);

  /* lower_bound gives us an iterator to the position at which the 
   * value "which" could be inserted without violating the sort order.
   *
   * If we end up at the end(), our song is in the last hunk and we
   * can just back up from the end of the list.
   *
   * If we end up somewhere else, "i" points to the first score sum
   * that was equal or larger to the "which" value, so we've run past
   * the goal at that song and need to back up.
   *
   * Note that the i'th entry of the ScoreSums vector indexes the
   * running score sums through element i*IndexInterval of the Songs
   * vector, so having found a spot in ScoreSums, we can easily find
   * the corresponding location in Songs and SongScores.
   */

  unsigned search_pos;
  double search_score_sum;

  if (i == pimpl->ScoreSums.end())
  {
    search_score_sum = pimpl->TotalScoreSum;
    search_pos = pimpl->Songs.size() - 1;
  }
  else
  {
    search_score_sum = *i;
    search_pos = (i - pimpl->ScoreSums.begin()) * IndexInterval;
  }

  assert( search_score_sum > which );

  do
    search_score_sum -= pimpl->SongScores[search_pos];
  while ( which <= search_score_sum && --search_pos >= 0);

  if (search_pos < 0)
  {
    // Didn't find anything?  List must have been empty.
    assert(pimpl->Songs.size() == 0);
    return NULL;
  }

  assert(pimpl->SongScores[ search_pos ] != 0);

#ifndef NDEBUG
  {
    /* Verify through direct calculation that this is the right song.
     * Check the ScoreSums on the way up.
     */

    double current_score_sum = 0;

    for (unsigned index = 0; index < pimpl->SongScores.size(); index++)
    {
      double new_score_sum = current_score_sum + pimpl->SongScores[ index ];
      if ( current_score_sum <= which && which < new_score_sum) 
      {
// 	cout << "expecting " << index << ", got " << search_pos << endl;
// 	cout << "real score sum " << current_score_sum << ", guessed " << search_score_sum << endl;
	assert(index == search_pos);
      }

      if (index % IndexInterval == 0 )
      {
	double recorded_score_sum = pimpl->ScoreSums[ index / IndexInterval ];
// 	cout << "recorded score sum at index " << index << ":" << recorded_score_sum
// 	  << " vs. real: " << new_score_sum << endl;

	assert(fabs(recorded_score_sum - new_score_sum) < 1e-10);
      }
      current_score_sum = new_score_sum;
    }
  }
#endif // not NDEBUG

  /*  Change this song's rating to 0 to prevent re-selection.  To do
   *  this we have to change the entry in SongScores to 0 and also
   *  update the values in ScoreSums and TotalRatingSum.
   */

  // Just in case searchPos moved back past the previous index point,
  // move i to the first index point at or after search_pos.
  i = ( pimpl->ScoreSums.begin() + 
	( search_pos + IndexInterval - 1 ) / IndexInterval ); 

  float rating = pimpl->SongScores[ search_pos ];
  while (i != pimpl->ScoreSums.end())
    *i++ -= rating;

  pimpl->TotalScoreSum -= rating;
  pimpl->SongScores[ search_pos ] = 0;

  return pimpl->Songs[ search_pos ];
}




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
