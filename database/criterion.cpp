/*
madman - a music manager
Copyright (C) 2004-2005  Andreas Kloeckner

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




#include <qapplication.h>
#include <qfile.h>

#include "database/criterion.h"
#include "database/song_set.h"
#include "database/song.h"
#include "utility/scanner.h"





namespace
{
  const int TOKEN_FLAG_DEPRECATED = 1;

  const int TT_CRIT_PAREN_OPEN = 0;
  const int TT_CRIT_PAREN_CLOSE = 1;
  const int TT_CRIT_NOT = 2;
  const int TT_CRIT_AND = 3;
  const int TT_CRIT_OR = 4;

  const int TT_CRIT_ALBUM_CRIT = 100;
  const int TT_CRIT_ARTIST_CRIT = 101;
  const int TT_CRIT_PERFORMER_CRIT = 102;
  const int TT_CRIT_FILENAME_CRIT = 103;
  const int TT_CRIT_TITLE_CRIT = 104;
  const int TT_CRIT_GENRE_CRIT = 105;
  const int TT_CRIT_ANY_CRIT = 106;
  const int TT_CRIT_ALL_CRIT = 107;
  const int TT_CRIT_NONE_CRIT = 108;
  const int TT_CRIT_RATING_CRIT = 109;
  const int TT_CRIT_PLAYCOUNT_CRIT = 110;
  const int TT_CRIT_EXISTEDFORDAYS_CRIT = 111;
  const int TT_CRIT_FULLPLAYRATIO_CRIT = 112;
  const int TT_CRIT_UNIQUEID_CRIT = 113;
  const int TT_CRIT_UNRATED_CRIT = 114;
  const int TT_CRIT_FULLPLAYCOUNT_CRIT = 115;
  const int TT_CRIT_PARTIALPLAYCOUNT_CRIT = 116;
  const int TT_CRIT_LASTPLAYEDNDAYSAGO_CRIT = 117;
  const int TT_CRIT_YEAR_CRIT = 118;
  const int TT_CRIT_TRACKNUMBER_CRIT = 119;  
  const int TT_CRIT_PATHNAME_CRIT = 120;
  const int TT_CRIT_FILENAME_WITHOUT_PATH_CRIT = 121;
  const int TT_CRIT_MOOD_CRIT = 122;
  const int TT_CRIT_TEMPO_CRIT = 123;
  const int TT_CRIT_CUSTOM1_CRIT = 124;
  const int TT_CRIT_CUSTOM2_CRIT = 125;
  const int TT_CRIT_CUSTOM3_CRIT = 126;


  const int TT_CRIT_COMPLETE_MATCH = 200;
  const int TT_CRIT_SUBSTRING_MATCH = 201;
  const int TT_CRIT_RE_MATCH = 202;
  const int TT_CRIT_FUZZY_MATCH = 203;
  const int TT_CRIT_STARTSWITH_MATCH = 204;

  const int TT_CRIT_NUM_LESS_MATCH = 204;
  const int TT_CRIT_NUM_LESSEQUAL_MATCH = 205;
  const int TT_CRIT_NUM_EQUAL_MATCH = 206;
  const int TT_CRIT_NUM_GREATER_MATCH = 207;
  const int TT_CRIT_NUM_GREATEREQUAL_MATCH = 208;

  const int TT_CRIT_STRING = 300;
  const int TT_CRIT_QUOTED_STRING = 301;



  class tCriterionScanner : public tScanner
  {
    public:
      tCriterionScanner()
      {
	addTokenDescriptor(TT_CRIT_PAREN_OPEN, "\\(");
	addTokenDescriptor(TT_CRIT_PAREN_CLOSE, "\\)");
	addTokenDescriptor(TT_CRIT_NOT, "\\!");
	addTokenDescriptor(TT_CRIT_AND, "\\&");
	addTokenDescriptor(TT_CRIT_OR, "\\|");
	addTokenDescriptor(TT_CRIT_ALBUM_CRIT, "\\~album");
	addTokenDescriptor(TT_CRIT_ARTIST_CRIT, "\\~artist");
	addTokenDescriptor(TT_CRIT_PERFORMER_CRIT, "\\~performer");
	addTokenDescriptor(TT_CRIT_FILENAME_CRIT, "\\~filename");
	addTokenDescriptor(TT_CRIT_TITLE_CRIT, "\\~title");
	addTokenDescriptor(TT_CRIT_GENRE_CRIT, "\\~genre");
	addTokenDescriptor(TT_CRIT_ANY_CRIT, "\\~any");
	addTokenDescriptor(TT_CRIT_RATING_CRIT, "\\~rating");
	addTokenDescriptor(TT_CRIT_PLAYCOUNT_CRIT, "\\~play_count");
	addTokenDescriptor(TT_CRIT_EXISTEDFORDAYS_CRIT, "\\~existed_for_days");
	addTokenDescriptor(TT_CRIT_FULLPLAYRATIO_CRIT, "\\~full_play_ratio");
	addTokenDescriptor(TT_CRIT_UNIQUEID_CRIT, "\\~uniqueid");
	addTokenDescriptor(TT_CRIT_UNRATED_CRIT, "\\~unrated");
	addTokenDescriptor(TT_CRIT_FULLPLAYCOUNT_CRIT, "\\~full_play_count");
	addTokenDescriptor(TT_CRIT_PARTIALPLAYCOUNT_CRIT, "\\~partial_play_count");
	addTokenDescriptor(TT_CRIT_LASTPLAYEDNDAYSAGO_CRIT, "\\~last_played_n_days_ago");
	addTokenDescriptor(TT_CRIT_YEAR_CRIT, "\\~year");
	addTokenDescriptor(TT_CRIT_TRACKNUMBER_CRIT, "\\~track_number");
	addTokenDescriptor(TT_CRIT_PATHNAME_CRIT, "\\~pathname");
	addTokenDescriptor(TT_CRIT_FILENAME_WITHOUT_PATH_CRIT, "\\~filename_without_path");

	// deprecated non-underscore criteria
	addTokenDescriptor(TT_CRIT_PLAYCOUNT_CRIT, "\\~playcount", false, false, TOKEN_FLAG_DEPRECATED);
	addTokenDescriptor(TT_CRIT_EXISTEDFORDAYS_CRIT, "\\~existedfordays", false, false, TOKEN_FLAG_DEPRECATED);
	addTokenDescriptor(TT_CRIT_FULLPLAYRATIO_CRIT, "\\~fullplayratio", false, false, TOKEN_FLAG_DEPRECATED);

	addTokenDescriptor(TT_CRIT_ALL_CRIT, "\\~all(\b|$)");
	addTokenDescriptor(TT_CRIT_NONE_CRIT, "\\~none(\b|$)");

	addTokenDescriptor(TT_CRIT_COMPLETE_MATCH, "complete\\:");
	addTokenDescriptor(TT_CRIT_SUBSTRING_MATCH, "substring\\:");
	addTokenDescriptor(TT_CRIT_STARTSWITH_MATCH, "startswith\\:");
	addTokenDescriptor(TT_CRIT_RE_MATCH, "re\\:");
	addTokenDescriptor(TT_CRIT_FUZZY_MATCH, "fuzzy\\:");

	addTokenDescriptor(TT_CRIT_NUM_LESSEQUAL_MATCH, "\\<\\=");
	addTokenDescriptor(TT_CRIT_NUM_GREATEREQUAL_MATCH, "\\>\\=");
	addTokenDescriptor(TT_CRIT_NUM_LESS_MATCH, "\\<");
	addTokenDescriptor(TT_CRIT_NUM_EQUAL_MATCH, "\\=");
	addTokenDescriptor(TT_CRIT_NUM_GREATER_MATCH, "\\>");

	addTokenDescriptor(TT_CRIT_FUZZY_MATCH, "fuzzy\\:");
	addTokenDescriptor(TT_CRIT_STRING, "(\\w|[-.,/\\\\])+");
	addTokenDescriptor(TT_CRIT_QUOTED_STRING, "\\\"([^\"]|\\\\\")*\\\"");
	//
	// whitespace
	addTokenDescriptor(0, "[ \n\r\t]+", true, true);
	// comments
	addTokenDescriptor(0, "\\{[^}]*\\}", true, true);
	
	addTokenDescriptor(TT_CRIT_MOOD_CRIT, "\\~mood");
	addTokenDescriptor(TT_CRIT_TEMPO_CRIT, "\\~tempo");
	addTokenDescriptor(TT_CRIT_CUSTOM1_CRIT, "\\~custom1");
	addTokenDescriptor(TT_CRIT_CUSTOM2_CRIT, "\\~custom2");
	addTokenDescriptor(TT_CRIT_CUSTOM3_CRIT, "\\~custom3");
      }
  } CriterionScanner;
}




// criteria -------------------------------------------------------------------
namespace
{
  class tTextCriterion 
  {
    public:
      virtual ~tTextCriterion()
      { }
      virtual float match(const QString &str) = 0;
  };

  class tNumericCriterion 
  {
    public:
      virtual ~tNumericCriterion()
      { }
      virtual float match(double x) = 0;
  };






// text criteria --------------------------------------------------------------
  class tFuzzyCriterion : public tTextCriterion
  {
      typedef hash_map<QString, int, hash_QString> tnGramTable;

      tnGramTable		SmallNGramTable;
      tnGramTable		BigNGramTable;
      QString		Pattern;
      int			PatternLength;

      static int const SmallN = 3;
      static int const BigN = 5;

    public:
      tFuzzyCriterion(const QString &op);
      float match(const QString &str);
  };




  // only for odd n
#define FOREACH_N_GRAM(STRING, N, OPERATION) \
    { \
      int strlength = STRING.length(); \
      int nhalf = (N) / 2; \
      { \
	QString ngram = " " + STRING.mid(0, N - 1); \
	OPERATION; \
      } \
      int i = nhalf; \
      while (i < strlength - nhalf) \
      { \
	QString ngram = STRING.mid(i - nhalf, N);\
	OPERATION; \
	if (STRING[ i + nhalf ].isSpace()) \
	  i += (N) - 1; \
	else \
	  i++; \
      } \
      { \
	QString ngram = STRING.mid(strlength - ((N) - 1), (N) - 1) + " "; \
	OPERATION; \
      } \
    }




  tFuzzyCriterion::tFuzzyCriterion(const QString &op)
  {
    Pattern = op;
    PatternLength = op.length();

    FOREACH_N_GRAM(op, SmallN, 
	SmallNGramTable[ ngram ] = 1;);
    FOREACH_N_GRAM(op, BigN, 
	BigNGramTable[ ngram ] = 1;);
  }




  float tFuzzyCriterion::match(const QString &str)
  {
    if ((int) str.length() < SmallN)
      return 0;

    int small_matches = 0, big_matches = 0;

    FOREACH_N_GRAM(str, SmallN, 
      if (SmallNGramTable.find(ngram) != SmallNGramTable.end())
	small_matches++;
	);
    FOREACH_N_GRAM(str, BigN, 
      if (BigNGramTable.find(ngram) != BigNGramTable.end())
	big_matches++;);

    float result = float(small_matches * SmallN + big_matches * BigN)/
      float(SmallNGramTable.size() * SmallN + BigNGramTable.size() * BigN);
    if (result > 1)
      return 1;
    else
      return result;
  }




  class tSubstringCriterion : public tTextCriterion
  {
    private:
      QString Operand;
    public:
      tSubstringCriterion(const QString &op)
      : Operand(op)
      { }
      float match(const QString &str)
      { 
	return str.find(Operand, 0, /* case sensitive */ false) != -1 ? 1 : 0; 
      }
  };




  class tStartsWithCriterion : public tTextCriterion
  {
    private:
      QString Operand;
    public:
      tStartsWithCriterion(const QString &op)
      : Operand(op.lower())
      { }
      float match(const QString &str)
      { 
	return str.lower().left(Operand.length()) == Operand;
      }
  };




  class tCompleteCriterion : public tTextCriterion
  {
    private:
      QString Operand;
    public:
      tCompleteCriterion(const QString &op)
      : Operand(op.lower())
      { }
      float match(const QString &str)
      { 
	return 
	  (str.lower().startsWith( Operand) &&
	    str.length() == Operand.length()) ? 1 : 0; 
      }
  };




  class tRegexCriterion : public tTextCriterion {
    private:
      QRegExp Operand;
    public:
      tRegexCriterion(const QString &op)
      : Operand(op)
      { 
	Operand.setCaseSensitive(false);
      }
      float match(const QString &str)
      { 
	return Operand.search(str) ? 1 : 0; 
      }
  };




  // numeric criteria -----------------------------------------------------------
  class tRelationalNumericCriterion : public tNumericCriterion {
    private:
      float	ReferenceNumber;
      int		TokenType;

    public:
      tRelationalNumericCriterion(double reference, int token_type)
      : ReferenceNumber(reference), TokenType(token_type)
      { }
      float match(double x)
      {
	switch (TokenType)
	{
	  case TT_CRIT_NUM_LESS_MATCH:
	    return x < ReferenceNumber ? 1 : 0;
	  case TT_CRIT_NUM_LESSEQUAL_MATCH:
	    return x <= ReferenceNumber ? 1 : 0;
	  case TT_CRIT_NUM_EQUAL_MATCH:
	    return x == ReferenceNumber ? 1 : 0;
	  case TT_CRIT_NUM_GREATER_MATCH:
	    return x > ReferenceNumber ? 1 : 0;
	  case TT_CRIT_NUM_GREATEREQUAL_MATCH:
	    return x >= ReferenceNumber ? 1 : 0;
	  default:
	    return 0;
	}      
      }
  };




  // criteria -------------------------------------------------------------------
  class tAnyCriterion : public tCriterion
  {
      auto_ptr<tTextCriterion>		TextCriterion;
    public:
      tAnyCriterion(tTextCriterion *tc)
      : TextCriterion(tc) 
      { }
      float matchDegree(const tSong *song) const
      {
	return TextCriterion->match(song->artist()) ||
	  TextCriterion->match(song->performer()) ||
	  TextCriterion->match(song->title()) ||
	  TextCriterion->match(song->album()) ||
  	  TextCriterion->match(song->mood()) ||
	  TextCriterion->match(song->tempo()) ||
	  TextCriterion->match(song->custom1()) ||
	  TextCriterion->match(song->custom2()) ||
	  TextCriterion->match(song->custom3()) ||
	  TextCriterion->match(QString::fromUtf8(song->filenameOnly().c_str()));
      }
  };




  class tArtistCriterion : public tCriterion
  {
      auto_ptr<tTextCriterion>		TextCriterion;
    public:
      tArtistCriterion(tTextCriterion *tc)
      : TextCriterion(tc) 
      { }
      float matchDegree(const tSong *song) const
      {
	return TextCriterion->match(song->artist());
      }
  };




  class tPerformerCriterion : public tCriterion
  {
      auto_ptr<tTextCriterion>		TextCriterion;
    public:
      tPerformerCriterion(tTextCriterion *tc)
      : TextCriterion(tc) 
      { }
      float matchDegree(const tSong *song) const
      {
	return TextCriterion->match(song->performer());
      }
  };




  class tAlbumCriterion : public tCriterion
  {
      auto_ptr<tTextCriterion>		TextCriterion;
    public:
      tAlbumCriterion(tTextCriterion *tc)
      : TextCriterion(tc) 
      { }
      float matchDegree(const tSong *song) const
      {
	return TextCriterion->match(song->album());
      }
  };




  class tTitleCriterion : public tCriterion
  {
      auto_ptr<tTextCriterion>		TextCriterion;
    public:
      tTitleCriterion(tTextCriterion *tc)
      : TextCriterion(tc) 
      { }
      float matchDegree(const tSong *song) const
      {
	return TextCriterion->match(song->title());
      }
  };




  class tGenreCriterion : public tCriterion
  {
      auto_ptr<tTextCriterion>		TextCriterion;
    public:
      tGenreCriterion(tTextCriterion *tc)
      : TextCriterion(tc) 
      { }
      float matchDegree(const tSong *song) const
      {
	return TextCriterion->match(song->genre());
      }
  };




  class tFilenameCriterion : public tCriterion
  {
      auto_ptr<tTextCriterion>		TextCriterion;
    public:
      tFilenameCriterion(tTextCriterion *tc)
      : TextCriterion(tc) 
      { }
      float matchDegree(const tSong *song) const
      {
	return TextCriterion->match(
          QFile::decodeName(song->filename().c_str()));
      }
  };




  class tFilenameWithoutPathCriterion : public tCriterion
  {
      auto_ptr<tTextCriterion>		TextCriterion;
    public:
      tFilenameWithoutPathCriterion(tTextCriterion *tc)
      : TextCriterion(tc) 
      { }
      float matchDegree(const tSong *song) const
      {
	return TextCriterion->match(QFile::decodeName(song->filenameOnly().c_str()));
      }
  };




  class tPathnameCriterion : public tCriterion
  {
      auto_ptr<tTextCriterion>		TextCriterion;
    public:
      tPathnameCriterion(tTextCriterion *tc)
      : TextCriterion(tc) 
      { }
      float matchDegree(const tSong *song) const
      {
	return TextCriterion->match(QFile::decodeName(song->pathname().c_str()));
      }
  };




  class tRatingCriterion : public tCriterion
  {
      auto_ptr<tNumericCriterion>		NumericCriterion;
    public:
      tRatingCriterion(tNumericCriterion *nc)
      : NumericCriterion(nc) 
      { }
      float matchDegree(const tSong *song) const
      {
	return NumericCriterion->match(song->rating());
      }
  };




  class tUnratedCriterion : public tCriterion
  {
    public:
      float matchDegree(const tSong *song) const
      {
	return song->rating() == -1;
      }
  };




  class tPlayCountCriterion : public tCriterion
  {
      auto_ptr<tNumericCriterion>		NumericCriterion;
    public:
      tPlayCountCriterion(tNumericCriterion *nc)
      : NumericCriterion(nc) 
      { }
      float matchDegree(const tSong *song) const
      {
	return NumericCriterion->match(song->playCount());
      }
  };




  class tExistedForDaysCriterion : public tCriterion
  {
      auto_ptr<tNumericCriterion>		NumericCriterion;
    public:
      tExistedForDaysCriterion(tNumericCriterion *nc)
      : NumericCriterion(nc) 
      { }
      float matchDegree(const tSong *song) const
      {
	time_t now = time(NULL);
	time_t song_added = song->existsSince();
	double diff_in_days = (double) (now - song_added) / (3600. * 24);

	return NumericCriterion->match(diff_in_days);
      }
  };




  class tFullPlayRatioCriterion : public tCriterion
  {
      auto_ptr<tNumericCriterion>		NumericCriterion;
    public:
      tFullPlayRatioCriterion(tNumericCriterion *nc)
      : NumericCriterion(nc) 
      { }
      float matchDegree(const tSong *song) const
      {
	if (song->playCount() == 0)
	  return NumericCriterion->match(0);
	double ratio = (double) song->fullPlayCount() / (double) song->playCount();
	return NumericCriterion->match(ratio);
      }
  };




  class tUniqueIdCriterion : public tCriterion
  {
      auto_ptr<tNumericCriterion>		NumericCriterion;
    public:
      tUniqueIdCriterion(tNumericCriterion *nc)
      : NumericCriterion(nc) 
      { }
      float matchDegree(const tSong *song) const
      {
	return NumericCriterion->match(song->uniqueId());
      }
  };




  class tFullPlayCountCriterion : public tCriterion
  {
      auto_ptr<tNumericCriterion>		NumericCriterion;
    public:
      tFullPlayCountCriterion(tNumericCriterion *nc)
      : NumericCriterion(nc) 
      { }
      float matchDegree(const tSong *song) const
      {
	return NumericCriterion->match(song->fullPlayCount());
      }
  };




  class tPartialPlayCountCriterion : public tCriterion
  {
      auto_ptr<tNumericCriterion>		NumericCriterion;
    public:
      tPartialPlayCountCriterion(tNumericCriterion *nc)
      : NumericCriterion(nc) 
      { }
      float matchDegree(const tSong *song) const
      {
	return NumericCriterion->match(song->partialPlayCount());
      }
  };




  class tLastPlayedNDaysAgoCriterion : public tCriterion
  {
      auto_ptr<tNumericCriterion>		NumericCriterion;
    public:
      tLastPlayedNDaysAgoCriterion(tNumericCriterion *nc)
      : NumericCriterion(nc) 
      { }
      float matchDegree(const tSong *song) const
      {
	time_t now = time(NULL);
	time_t song_last_played = song->lastPlayed();
	double diff_in_days = (double) (now - song_last_played) / (3600. * 24);

	return NumericCriterion->match(diff_in_days);
      }
  };




  class tYearCriterion : public tCriterion
  {
      auto_ptr<tNumericCriterion>		NumericCriterion;
    public:
      tYearCriterion(tNumericCriterion *nc)
      : NumericCriterion(nc) 
      { }
      float matchDegree(const tSong *song) const
      {
	QRegExp extract_numbers("^\\s*([0-9]+)");
	if (extract_numbers.search(song->year()) != -1)
	{
	  int my_year = extract_numbers.cap(1).toInt();
	  return NumericCriterion->match(my_year);
	}
	else
	  return false;
      }
  };




  class tTrackNumberCriterion : public tCriterion
  {
      auto_ptr<tNumericCriterion>		NumericCriterion;
    public:
      tTrackNumberCriterion(tNumericCriterion *nc)
      : NumericCriterion(nc) 
      { }
      float matchDegree(const tSong *song) const
      {
	QRegExp extract_numbers("^\\s*([0-9]+)");
	if (extract_numbers.search(song->trackNumber()) != -1)
	{
	  int my_track_number = extract_numbers.cap(1).toInt();
	  return NumericCriterion->match(my_track_number);
	}
	else
	  return false;
      }
  };


  class tMoodCriterion : public tCriterion
  {
      auto_ptr<tTextCriterion>		TextCriterion;
    public:
      tMoodCriterion(tTextCriterion *tc)
      : TextCriterion(tc) 
      { }
      float matchDegree(const tSong *song) const
      {
	return TextCriterion->match(song->mood());
      }
  };


  class tTempoCriterion : public tCriterion
  {
      auto_ptr<tTextCriterion>		TextCriterion;
    public:
      tTempoCriterion(tTextCriterion *tc)
      : TextCriterion(tc) 
      { }
      float matchDegree(const tSong *song) const
      {
	return TextCriterion->match(song->tempo());
      }
  };

  class tCustom1Criterion : public tCriterion
  {
      auto_ptr<tTextCriterion>		TextCriterion;
    public:
      tCustom1Criterion(tTextCriterion *tc)
      : TextCriterion(tc) 
      { }
      float matchDegree(const tSong *song) const
      {
	return TextCriterion->match(song->custom1());
      }
  };

  class tCustom2Criterion : public tCriterion
  {
      auto_ptr<tTextCriterion>		TextCriterion;
    public:
      tCustom2Criterion(tTextCriterion *tc)
      : TextCriterion(tc) 
      { }
      float matchDegree(const tSong *song) const
      {
	return TextCriterion->match(song->custom2());
      }
  };
  
  class tCustom3Criterion : public tCriterion
  {
      auto_ptr<tTextCriterion>		TextCriterion;
    public:
      tCustom3Criterion(tTextCriterion *tc)
      : TextCriterion(tc) 
      { }
      float matchDegree(const tSong *song) const
      {
	return TextCriterion->match(song->custom3());
      }
  };

  
  
  class tTrueCriterion : public tCriterion
  {
    public:
      float matchDegree(const tSong * /*song*/) const
      {
	return 1;
      }
  };




  class tFalseCriterion : public tCriterion
  {
    public:
      float matchDegree(const tSong * /*song*/) const
      {
	return false;
      }
  };
}




// tInSetCriterion ------------------------------------------------------------
tInSetCriterion::tInSetCriterion(tSongSet *ss)
: SongSet(ss)
{
}




float tInSetCriterion::matchDegree(const tSong *song) const
{
  return SongSet->containsSong(song->uniqueId());
}




// parsing --------------------------------------------------------------------
namespace
{
#define ADVANCE \
  first++; \
  if (first == last) \
    throw runtime_error("unexpected end of expression");
#define EXPECT(WHAT,STRINGIFIED) \
  if (first == last) \
    throw runtime_error("unexpected end of expression"); \
  if (first->TokenType != WHAT) \
    throw tRuntimeError(qApp->translate("ErrorMessages", "Expected '%1', found '%2'.").arg(STRINGIFIED).arg(first->Text));





  QString getText(tTokenList::iterator &first, tTokenList::iterator /*last*/)
  {
    if (first->TokenType == TT_CRIT_STRING)
      return (first++)->Text;
    else if (first->TokenType == TT_CRIT_QUOTED_STRING)
    {
      QString result = (first++)->Text;
      result = result.mid(1, result.length() - 2);
      // unquote string
      result.replace(QRegExp("\\\\\\\""), "\"");
      return result;
    }
    else
    {
      throw tRuntimeError(qApp->translate("ErrorMessages", "Expected string expression, found: '%1'.").arg(first->Text));
    }
  }




  tNumericCriterion *parseNumericCriterion(tTokenList::iterator &first, tTokenList::iterator last)
  {
    double ref = 0;
    int token_type = TT_CRIT_NUM_EQUAL_MATCH; 
    switch (first->TokenType)
    {
      case TT_CRIT_NUM_LESS_MATCH:
      case TT_CRIT_NUM_LESSEQUAL_MATCH:
      case TT_CRIT_NUM_EQUAL_MATCH:
      case TT_CRIT_NUM_GREATER_MATCH:
      case TT_CRIT_NUM_GREATEREQUAL_MATCH:
	token_type = first->TokenType;
	ADVANCE;
      default:
	  bool ok;
	  QString ref_num_string = getText(first, last);
	  ref = ref_num_string.toDouble(&ok);
	  if (!ok)
	    throw tRuntimeError(qApp->translate("ErrorMessages", "Expected number, found: %1") .arg(ref_num_string));
    }
    return new tRelationalNumericCriterion(ref, token_type);
  }




  tTextCriterion *parseTextCriterion(tTokenList::iterator &first, tTokenList::iterator last)
  {
    if (first->TokenType == TT_CRIT_COMPLETE_MATCH)
    {
      ADVANCE;
      return new tCompleteCriterion(getText(first, last));
    }
    if (first->TokenType == TT_CRIT_SUBSTRING_MATCH)
    {
      ADVANCE;
      return new tSubstringCriterion(getText(first, last));
    }
    if (first->TokenType == TT_CRIT_STARTSWITH_MATCH)
    {
      ADVANCE;
      return new tStartsWithCriterion(getText(first, last));
    }
    if (first->TokenType == TT_CRIT_RE_MATCH)
    {
      ADVANCE;
      return new tRegexCriterion(getText(first, last));
    }
    if (first->TokenType == TT_CRIT_FUZZY_MATCH)
    {
      ADVANCE;
      return new tFuzzyCriterion(getText(first, last));
    }
    QString pattern = getText(first, last);
    return new tSubstringCriterion(pattern);
  }




  tCriterion *parseAtomicCriterion(tTokenList::iterator &first, tTokenList::iterator last)
  {
#define PARSE_PAREN_TEXT_CRITERION(TOKENTYPE, CLASSNAME)\
    if (first->TokenType == TOKENTYPE) \
    { \
      if (first->Flags & TOKEN_FLAG_DEPRECATED) \
      { \
	cerr << "*** WARNING *** the criterion " << first->Text \
	  << " has been deprecated." << endl \
	  << "Please take a look at the online help to find the new syntax." << endl; \
      } \
      ADVANCE; \
      EXPECT(TT_CRIT_PAREN_OPEN, "')'"); \
      ADVANCE; \
      auto_ptr<tCriterion> result(new CLASSNAME(parseTextCriterion(first, last))); \
      EXPECT(TT_CRIT_PAREN_CLOSE, "')'"); \
      first++; \
      return result.release(); \
    }
    PARSE_PAREN_TEXT_CRITERION(TT_CRIT_ALBUM_CRIT, tAlbumCriterion);
    PARSE_PAREN_TEXT_CRITERION(TT_CRIT_ARTIST_CRIT, tArtistCriterion);
    PARSE_PAREN_TEXT_CRITERION(TT_CRIT_PERFORMER_CRIT, tPerformerCriterion);
    PARSE_PAREN_TEXT_CRITERION(TT_CRIT_FILENAME_CRIT, tFilenameCriterion);
    PARSE_PAREN_TEXT_CRITERION(TT_CRIT_PATHNAME_CRIT, tPathnameCriterion);
    PARSE_PAREN_TEXT_CRITERION(TT_CRIT_FILENAME_WITHOUT_PATH_CRIT, tFilenameWithoutPathCriterion);
    PARSE_PAREN_TEXT_CRITERION(TT_CRIT_TITLE_CRIT, tTitleCriterion);
    PARSE_PAREN_TEXT_CRITERION(TT_CRIT_GENRE_CRIT, tGenreCriterion);
    PARSE_PAREN_TEXT_CRITERION(TT_CRIT_MOOD_CRIT, tMoodCriterion);
    PARSE_PAREN_TEXT_CRITERION(TT_CRIT_TEMPO_CRIT, tTempoCriterion);
    PARSE_PAREN_TEXT_CRITERION(TT_CRIT_CUSTOM1_CRIT, tCustom1Criterion);
    PARSE_PAREN_TEXT_CRITERION(TT_CRIT_CUSTOM2_CRIT, tCustom2Criterion);
    PARSE_PAREN_TEXT_CRITERION(TT_CRIT_CUSTOM3_CRIT, tCustom3Criterion);
    PARSE_PAREN_TEXT_CRITERION(TT_CRIT_ANY_CRIT, tAnyCriterion);

#define PARSE_PAREN_NUM_CRITERION(TOKENTYPE, CLASSNAME)\
    if (first->TokenType == TOKENTYPE) \
    { \
      if (first->Flags & TOKEN_FLAG_DEPRECATED) \
      { \
	cerr << "*** WARNING *** the criterion " << first->Text \
	  << " has been deprecated." << endl \
	  << "Please take a look at the online help to find the new syntax." << endl; \
      } \
      ADVANCE; \
      EXPECT(TT_CRIT_PAREN_OPEN, "'('"); \
      ADVANCE; \
      auto_ptr<tCriterion> result(new CLASSNAME(parseNumericCriterion(first, last))); \
      EXPECT(TT_CRIT_PAREN_CLOSE, "')'"); \
      first++; \
      return result.release(); \
    }
    PARSE_PAREN_NUM_CRITERION(TT_CRIT_RATING_CRIT, tRatingCriterion);
    PARSE_PAREN_NUM_CRITERION(TT_CRIT_PLAYCOUNT_CRIT, tPlayCountCriterion);
    PARSE_PAREN_NUM_CRITERION(TT_CRIT_EXISTEDFORDAYS_CRIT, tExistedForDaysCriterion);
    PARSE_PAREN_NUM_CRITERION(TT_CRIT_FULLPLAYRATIO_CRIT, tFullPlayRatioCriterion);
    PARSE_PAREN_NUM_CRITERION(TT_CRIT_UNIQUEID_CRIT, tUniqueIdCriterion);
    PARSE_PAREN_NUM_CRITERION(TT_CRIT_FULLPLAYCOUNT_CRIT, tFullPlayCountCriterion);
    PARSE_PAREN_NUM_CRITERION(TT_CRIT_PARTIALPLAYCOUNT_CRIT, tPartialPlayCountCriterion);
    PARSE_PAREN_NUM_CRITERION(TT_CRIT_LASTPLAYEDNDAYSAGO_CRIT, tLastPlayedNDaysAgoCriterion);
    PARSE_PAREN_NUM_CRITERION(TT_CRIT_YEAR_CRIT, tYearCriterion);
    PARSE_PAREN_NUM_CRITERION(TT_CRIT_TRACKNUMBER_CRIT, tTrackNumberCriterion);

    if (first->TokenType == TT_CRIT_ALL_CRIT)
    {
      first++;
      return new tTrueCriterion;
    }
    if (first->TokenType == TT_CRIT_NONE_CRIT)
    {
      first++;
      return new tFalseCriterion;
    }
    if (first->TokenType == TT_CRIT_UNRATED_CRIT)
    {
      first++;
      return new tUnratedCriterion;
    }

    return new tAnyCriterion(parseTextCriterion(first, last));
  }




  const int PRECEDENCE_UNARY = 200;
  const int PRECEDENCE_AND = 150;
  const int PRECEDENCE_OR = 100;
  tCriterion *parseCriterion(tTokenList::iterator &first, tTokenList::iterator last, const int min_precedence = 0)
  {
    QString ft = first->Text;
    auto_ptr<tCriterion> op1;
    if (first->TokenType == TT_CRIT_PAREN_OPEN)
    {
      ADVANCE;
      auto_ptr<tCriterion> temp_result(parseCriterion(first, last));
      op1 = temp_result;
      EXPECT(TT_CRIT_PAREN_CLOSE, "')'");
      first++;
    }
    else if (first->TokenType == TT_CRIT_NOT && min_precedence <= PRECEDENCE_UNARY)
    {
      ADVANCE;
      auto_ptr<tCriterion> op(parseCriterion(first, last, PRECEDENCE_UNARY));
      auto_ptr<tCriterion> temp_result(new tNotCriterion(op.get()));
      op.release();
      op1 = temp_result;
    }
    else 
    {
      auto_ptr<tCriterion> temp_result(parseAtomicCriterion(first, last));
      op1 = temp_result;
    }
    
    bool made_progress = true;
    while (first != last && made_progress)
    {
      made_progress = false;
      if (first-> TokenType == TT_CRIT_AND && min_precedence <= PRECEDENCE_AND)
      {
	ADVANCE;
	auto_ptr<tCriterion> temp_result(
	    new tAndCriterion(op1.get(), parseCriterion(first, last, PRECEDENCE_AND)));
	op1.release();
	op1 = temp_result;
	made_progress = true;
      }
      else if (first-> TokenType == TT_CRIT_OR && min_precedence <= PRECEDENCE_OR)
      {
	ADVANCE;
	auto_ptr<tCriterion> temp_result(
	    new tOrCriterion(op1.get(), parseCriterion(first, last, PRECEDENCE_OR)));
        op1.release();
	op1 = temp_result;
	made_progress = true;
      }
      else if (min_precedence <= PRECEDENCE_AND)
      {
	try
	{
	  tTokenList::iterator myfirst = first;
	  auto_ptr<tCriterion> temp_result(
	      new tAndCriterion(op1.get(), parseCriterion(myfirst, last, PRECEDENCE_AND)));
	  op1.release();
	  op1 = temp_result;
	  made_progress = true;
	  first = myfirst;
	}
	catch (...)
	{
	}
      }
    }

    return op1.release();
  }
}




auto_ptr<tCriterion> parseCriterion(QString criterion)
{
  tTokenList tokens;
  CriterionScanner.scan(criterion, tokens);
  tTokenList::iterator first = tokens.begin(), last = tokens.end();

  if (first == last)
    return auto_ptr<tCriterion>(new tTrueCriterion);
  auto_ptr<tCriterion> result(
      parseCriterion(first, last));

  if (first != last)
    throw tRuntimeError(qApp->translate("ErrorMessages", "Unrecognized tokens at end of criterion"));

  return result;
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

