<? 
  include( "include.php" );
  madman_header("Tour page 2");
  madman_menu();
  madman_begin_main();
?>
  <a href="tour1.php">&lt;&lt; previous</a> &middot;
  <a href="index.php">home &gt;&gt;</a><p>

  <b>A good split.</b><br>
  madman's UI is split in half: The top half shows you the contents of your
  music library and the results of your searches, while the bottom half 
  is the place for your personal playlists. By dragging the horizontal
  splitter bar either all the way to the top (until it snaps in) or all
  the way to the bottom, you can see either the library or the playlist
  half exclusively, hiding the other.
  <p>
  
  <img src="tour2_list_tree.png" class="TourShotLeft" alt="screenshot">
  <b>Leaving your mark.</b><br>
  Your playlists, listed in the bottom left window, may be arranged into
  a tree (like folders and files) by just dragging and dropping them on top of
  each other. Once you create a new playlist, you can populate it by just
  dragging songs from the library down into your playlist. Better yet, 
  you can add whole slews of songs by changing the playlist "criterion".
  For example, by putting "morrissette" into the criterion field, you
  automatically have all the songs by Alanis Morrissette included in the
  list. Also, unlike other product's "smart" playlists, madman allows
  you to remove songs from them, add others, put them in order 
  (by drag'n'drop), and it will actually remember that till the next time
  you use the playlist!
  <p>

  <b>Never forget.</b><br>
  If you have created playlists previously and don't want to lose them,
  don't worry. Find the "Import M3U" or "Import current player playlist"
  items in the right click menu of the playlist view. (bottom right).
  You should definitely note that the right click menu of the library 
  view and the playlist view are actually different.<p>

  <img src="tour2_criteria.png" class="TourShotRight" alt="screenshot">
  <b>Criterially speaking.</b><br>
  Here are some examples on how to write your criteria:
  <ul>
  <li> <tt>man moon</tt>: Songs that contain both the words "man" and "moon"
    in one of their fields.
  <li> <tt>man|moon</tt>: Songs that contain the word "man" or the word "moon"
    in one of their fields.
  <li> <tt>"3 doors down"</tt>: Just like on Google, quoting ensures that
    you only get songs that contain the phrase "3 doors down" in succession.
  <li> <tt>~artist(complete:creed)</tt>: That's how you say that you want
    Creed, but not Creedence Clearwater Revival.
  </ul>
  And here are some smart ideas for playlists that you could create:
  <ul>
  <li> <b>Absolute favorites:</b> <tt>~rating(&gt;=4)</tt>
  <li> <b>Can't stand to listen to them:</b> <tt>~fullplayratio(&lt;0.3)&amp;~playcount(&gt;3)</tt>
  <li> <b>Always finish listening to them:</b> <tt>~fullplayratio(&gt;0.5)&amp;~playcount(&gt;3)</tt>
  <li> <b>Never listened:</b> <tt>~playcount(=0)</tt>
  <li> <b>Love songs:</b> <tt>~title(love)|~album(love)</tt>
  </ul>

  madman's help browser has more information on how to write
  your own criteria.<p>
  
  <b>Get your fix.</b><br>
  Now you might feel inclined to head over to the 
  <a href="download.php">download page</a>.
  I hope you'll have as much fun with madman as I had developing
  it.<p>

  <i>Andreas</i>
  <p>

  <a href="tour1.php">&lt;&lt; previous</a> &middot;
  <a href="index.php">home &gt;&gt;</a><p>
<?
  madman_lastchanged( "Thu Jun 19 12:29:16 CEST 2003" );
  madman_end_main();
  madman_footer();
?>
