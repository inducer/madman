<? 
  include( "include.php" );
  madman_header("FAQ");
  madman_menu();
  madman_begin_main();
?>
  <b>Q: Why doesn't madman implement a way to tag several songs at once?</b><p/>

  A: It does. See "Tag &gt; Repeat last retag on selection." on the context menu.<p/>

  <b>Q: I used to be able to just drag column headers to change the order in
  which the columns were displayed. I can't any more. What's up?</b><p/>

  A: I'm sorry. This is a Qt-braindead-ism that's hard to change from the 
  application level. For now, just hold <tt>Control</tt> while you drag and everything 
  should work like before.<p/>

  <b>Q: How do I write a plugin for madman?</b><p/>

  A: Go to the <tt>plugins</tt> directory in the madman source tree, there's
  a <tt>README</tt> file that has all the details as well as an example.<p/>

  <b>Q: Why doesn't madman have a builtin player?</b><p/>

  A: For now, I would like madman to focus on what it does best, and that
  is managing your music.
  Rather than improvising a player, I want to use proven code (XMMS) that
  works well for almost everybody. madman may become a player in its
  own right at some point, but no guarantees. That said, adding an
  extra player backend to madman is really easy, so there if there's 
  some player you want controlled, knock yourself out. :)<p/>

  <h2>General build questions</h2>

  <b>Q: Why does SCons say "No tool named 'qt'"?</b><p>

  Jon Burgess says, start SCons with "$PWD/scons.py" instead of 
  "./scons.py".

  <h2>Compiling from CVS</h2>

  Jon Burgess deserves a big thank you for figuring most of
  the stuff in this section, as well as for providing patches to make the life
  of all you CVS junkies out there a lot easier. ;)<p/>

  <b>Q: What about <tt>.ui</tt> files that refuse to be processed by Qt 3.1's
  uic?</b><p>

  A: There's a line at the top of these files that says something with "3.2".
  Change it to "3.1". It'll work after that.<p/>

  <b>Q: The configure script stubbornly thinks I don't have the Ogg Vorbis
  libraries installed. I know I do. Huh?</b><p/>

  A: <a href="http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=175858">This
  Debian bug report</a> might help you even if you don't use Debian. The 
  short version is that the configure template that comes with ogg is buggy
  and needs to be fixed.<p>

<?
  madman_lastchanged( 
    "Thu May 20, 2004"
  );
  
  madman_end_main();
  madman_footer();
?>

