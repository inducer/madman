<? 
  include( "include.php" );
  madman_header("Download");
  madman_menu();
  madman_begin_main();
?>
  <div class="separator">
    Prebuilt packages
  </div>
  Prebuilt packages for madman are available from the
  <a href="http://sourceforge.net/project/showfiles.php?group_id=59086">SourceForge download page</a>.<p>

  <b>Debian:</b> madman is available from Debian unstable, so everybody lucky enough to have Debian,
  just type <tt>apt-get install madman</tt>.<p>

  <b>Fedora Core 1:</b> available from the download page. If you use these pacakges,
  please note that you need a new Qt (>=3.3, I take it) from the 
  <a href="http://kde-redhat.sf.net">KDE-Redhat</a> project.<p>

  <b>Slackware</b> packages may be downloaded from
  <a href="http://www.slackcare.com/">SlackCare</a>.<p/>

  <b>NetBSD</b> packages may be downloaded from 
  <a href="ftp://ftp.netbsd.org/pub/NetBSD/packages/pkgsrc/audio/madman/README.html">pkgsrc</a>.<p/>

  <b>SuSE</b> packages can be found 
  <a href="http://ftp.gwdg.de/pub/linux/suse/apt/SuSE/9.1-i386/RPMS.suser-rbos/">here</a>.<p/>

  Packages for madman were contributed by
  <ul>
    <li> Jaako H Kyro (Fedora Core 1) </li>
    <li> Christian Hammers (Debian package maintainer)</li>
    <li> SlackCare (Slackware i686) </li>
    <li> Adam Ward (Slackware) - his packages were removed from SF because they had
    some permission problem, and are thus superseded by the Slackcare ones.</li>
    <li> Ove Sorensen (NetBSD)</li>
    <li> Robin Seidel (SuSE)</li>
  </ul>

  <div class="separator">
    Building from source
  </div>
  You have the following options:
  <ul>
    <li> The current stable release is 
    <a href="http://prdownloads.sourceforge.net/madman/madman-<? echo $madman_current_version; ?>.tar.gz?download">madman-<? echo $madman_current_version; ?>.tar.gz</a>.<br>

    <li>The current development release is 
    <a href="http://prdownloads.sourceforge.net/madman/madman-<? echo $madman_development_version; ?>.tar.gz?download">madman-<? echo $madman_development_version; ?>.tar.gz</a>.<br>

    <li> The code as it is being developed can be obtained from my arch repository. 
    These three commands suffice to get you a version:
    <pre>
      $ tla my-id "I. R. Baboon &lt;irbaboon@cow-n-chicken.net&gt;"
      $ tla register-archive http://tiker.net/archives/2004-public
      $ tla get inform@tiker.net--2004-public/madman--production--1.0 madman
    </pre>
    A simple
    <pre>
      $ tla update
    </pre>
    gets you updated. Of course, you need <a href="http://gnuarch.org/">tla</a> installed.
    <li> Older releases may be obtained from the 
    <a href="http://sourceforge.net/project/showfiles.php?group_id=59086">SourceForge download page</a>.<p>
  </ul>

  In order to successfully build madman, you'll need the following packages,
  including their corresponding development components:
  <ul>
    <li> Trolltech's <a href="http://www.trolltech.com/">Qt</a> Version 3.1 or newer.
    <li> The <a href="http://www.xiph.org/ogg/vorbis/">Ogg Vorbis libraries</a> Version 1.0 or newer.
    <li> <a href="http://www.underbit.com/products/mad/">libid3tag</a> Version 0.15.1b or newer.
    <li> <a href="http://www.xmms.org/">XMMS</a> Version 1.2.6 or newer.
  </ul>

  Kevin Wallace contributed a utility he uses to load madman playlists onto
  his iPod: <a href="mad2pl-0.1.tar.gz">mad2pl-0.1.tar.gz</a>.
  I guess this brings us one step closer to iTunes ;)

<?
  madman_lastchanged( "Thu May 20, 2004" );
  madman_end_main();
  madman_footer();
?>


