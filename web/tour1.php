<? 
  include( "include.php" );
  madman_header("Tour page 1");
  madman_menu();
  madman_begin_main();
?>
  <a href="index.php">&lt;&lt; home</a> &middot;
  <a href="tour2.php">next &gt;&gt;</a><p>

  <img src="tour1_rescan_orphans.png" class="TourShotRight" alt="screenshot">
  <b>Finding all your music.</b><br>
  All you need to do to start enjoying madman is tell it where you keep
  your music. It will politely ask the first time you start it. It will
  create an index of your music once. After that, all the data is stored
  in a madman database that is ready to be searched within seconds,
  madman will even remember which database you used last.<p>

  madman can read MP3s and the fairly recent Ogg Vorbis file format. 
  Vorbis files usually get much higher quality into way smaller files. 
  So that's one big bonus.<p>

  If you add or remove music from your hard disk, madman will notice
  and update your database accordingly. (you may need to issue a "rescan"
  command - depending on how you set the "rescan on startup" option).<p>

  <img src="tour1_rate.png" class="TourShotLeft" alt="screenshot">
  <b>More information at your fingertips</b><br>
  madman keeps track of when you last played what song, how often, and if
  you let it play to the end. It also gives you the option to rate songs
  on an easy scale ranging from zero to five stars.
  Besides the usual Artist/Title/Album information, madman also has a 
  Performer field, which is useful if you like classical recordings or
  dance tracks that were remixed by a DJ. madman lets you customize which
  information you want to see and how you want it arranged. It will remember
  your decisions faithfully between runs.
  <p>

  <img src="tour1_retag.png" class="TourShotRight" alt="screenshot">
  <b>Need a tag editor?</b><br>
  Digital music from (*cough*) dubious sources often does not carry much
  tag information besides its filename. So, you need to change some tags,
  often lots of them. madman makes that easy, too. If you see a tag that 
  you don't like: Just "slow doubleclick" on it, and madman allows
  you to change it. Need to change more than one tag? Look for
  "Tag &gt; Repeat last retag on selection" on the right-click menu.
  madman can also tag from filenames if they contain sufficient information.
  There's also an AutoTag feature that will guess tags in an "intelligent"
  way based on the information in your database.
  A future version will even completely take away the load of retagging
  through the use of <a href="http://www.musicbrainz.org">MusicBrainz</a>.
  <p/>

  <b>Making playlists?</b><br/>
  One of madman's most powerful features is having automatic playlists.
  Some music managers allow you to have playlists built automatically through
  the use of rules. You enter "Everything by Aerosmith", and you get just that.
  madman can do that, too. But it can do even more: Playlists are not either rule-based
  (aka "vfolders") or handmade. madman combines the two into its notion of
  an automatic playlist. So just manually add a few songs that sound 
  "just like Aerosmith" to your Aerosmith playlist, and live 
  happily ever after. :)
  <p/>

  <a href="index.php">&lt;&lt; home</a> &middot;
  <a href="tour2.php">next &gt;&gt;</a><p>
<?
  madman_lastchanged( "Thu May 20, 2004" );
  madman_end_main();
  madman_footer();
?>

