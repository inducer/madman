<? 
  include( "include.php" );
  madman_header("Main Page");
  madman_menu();
  madman_begin_main();
?>
  <a href="download.php"><img src="shot_main.png" id="MainPageShot" 
  alt="Screenshot of madman with big-ass DOWNLOAD NOW"></a>

  <b>The one music manager to rule them all. :)</b><br>
  madman makes your digital music experience what it should have been
  from the start. Fun, not clumsy. Organized, not a mess. Cool, not
  technical. Let's face it: The "Open file" dialog is not an appropriate
  way to find the music that you like.
  Come and take a <a href="tour1.php">look</a> what madman can do for you.<p>


  <b>Selection at your fingertips.</b><br>
  madman automatically creates an index of all the digital music that you
  have. So, if you know you have that cool old Indie album lying around
  somewhere, but you just can't remember where, use madman's intelligent
  search features to see where it is. <a href="tour1.php">more...</a><p>

  <b>Join the madman community.</b><br>
  List: <a href="mailto:madman-discuss@lists.sourceforge.net">madman-discuss@lists.sourceforge.net</a>,
  click <a href="list.php">here</a> for information.<br>
  <a href="http://freshmeat.net/subscribe/35497">Get notified</a> when new releases
  of madman come out.

  <div style="background-color:rgb(200,200,200);clear:both;padding:5px;">
    <table><tr>
    <td valign="top">
      <span style="font-size:x-large;font-weight:bold;">News&nbsp;&nbsp;&nbsp;</span>
    </td>
    <td>
      <? include("news-items.html"); ?>
      <a href="http://news.tiker.net/taxonomy/term/1">more news...</a>
    </td>
    </tr></table>
  </div><p/>

  <b>Power and extensibility.</b><br>
  Once you find the music you've been looking for, madman easily plays
  it in your favorite MP3 player. You can also burn these songs to music
  or data CDs. And if that's not enough, writing a plugin for madman takes
  no more than writing a shell script. That way, you can upload music to 
  your portable MP3 player with one click of the mouse.
  <a href="download.php">feel it...</a><p>

  <b>Smart playlists.</b><br>
  Have you ever found it clumsy and tiring to manually add song by song 
  to your playlist, just to be able to listen to your all-time favorites?
  Sure enough, madman imports your previous work and manages existing
  playlists. But it also gives you a new and easier way to rack up the 
  music you like. It can use your ratings and listening habits to 
  write automatic, smart playlists for you. 
  <a href="tour2.php">more...</a><p>

  <b>Just the basics.</b><br>
  madman is <i>fast</i>. Designed from the ground up to avoid expensive 
  operations, it will not slow you down, even if your computer is not
  the newest. Even with several thousand songs in your database, madman 
  usually starts up in less than two or three seconds. madman's user
  interface is extremely simple. Almost everything works through
  right-click menus or drag'n'drop. Even compared to the last release,
  the new UI has been simplified a great deal - yet with no sacrifice
  in power. madman's C++ code base is clean and extensible, so if 
  you find yourself in need of one specific feature, you can usually
  add it in very little time.
  <a href="download.php">see it...</a><p>

  <b>Smokes the competition.</b><br>
  More than likely you will have heard of Apple's 
  <a href="http://www.apple.com/itunes">iTunes</a>.
  If you're a Windows user, you might have seen
  <a href="http://www.moodlogic.com">MoodLogic</a>,
  <a href="http://www.musicmatch.com">MusicMatch</a>
  or some others.
  If you already use Linux, you might have seen
  <a href="http://www.rhythmbox.org">Rhythmbox</a>,
  <a href="http://yammi.sf.net">Yammi</a>,
  <a href="http://developer.kde.org/~wheeler/juk.html">JuK</a>,
  <a href="http://mp3kult.sf.net">Mp3Kult</a> or
  <a href="http://www.zinf.org">Zinf</a>.
  madman is pretty much in the same vein as all these. 
  They all suck. All music managers suck. madman just 
  <a href="tour1.php">sucks less</a>. (Hm. Apologies to 
  <a href="http://www.mutt.org/">Michael R. Elkins</a> :))<p>
<?
  madman_lastchanged( "Thu May 20, 2004" );
  madman_end_main();
  madman_footer();
?>
