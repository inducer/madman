<?
madman_news_item("June 9, 2004",
  "<b>Package frenzy.</b> ".
  "Within minutes of each other, both Christian Hammers and Jaako H Kyro made ".
  "packages available for their respective distributions. So now, if you have ".
  "Fedora Core 1 or Debian, it's never been easier to get madman.<p>".
  "Thanks to the packagers!");
madman_news_item("May 25, 2004",
  "<b>Greetings from the night shift.</b> ".
  "Guess what. ".
  "CVS HEAD madman is now based on Scott Wheeler's TagLib. I've decided to delay ".
  "many other features planned for 0.94 and release as soon as possible. Hang ".
  "in there...");
madman_news_item("May 24, 2004",
  "<b>libid3tag.</b> If only anybody had told me this: I was just investigating ".
  "why some ID3 tags just aren't written for some MP3s. I stumbled upon this ".
  "piece of code, hidden deeply in the guts of libid3tag:<p>".
  "<code>".
  "&nbsp;&nbsp;/* hard general case: rewrite entire file */<br/>".
  "<br/>".
  "&nbsp;&nbsp;/* ... */<br/>".
  "done:<br/>".
  "&nbsp;&nbsp;return 0;<br/>".
  "</code><p>".
  "To those not fluent in C++, this means that libid3tag's implementer ".
  "chose to just forego the hard case and leave that as an exercise to the ".
  "interested reader. ".
  "Grrr. So, I guess it's going to be TagLib by Scott Wheeler.");
madman_news_item("May 24, 2004",
  "<b>More Slackware.</b> ".
  "<a href=\"http://www.slackcare.com/\">SlackCare</a> has also made Slackware packages, ".
  "which they claim are more correct than Adam's. Find the link to their packages on the ".
  "<a href=\"download.php\">download page</a>.<p/>".
  "<b>Email trouble.</b> ". 
  "Some people have complained that they were unable to get anything posted to the ".
  "mailing list or sent personally to me. I'll look into this, I'll let you know ".
  "right here if I find out what's causing this.<p/>".
  "<b>Bugs in 0.93.</b> ".
  "As was to be expected, there were a couple of bugs in 0.93. Here's the list so far:".
  "<ul>".
  "<li>The build system litters system directories with <tt>.sconsign</tt> files if you ".
  "let it.".
  "<li>The main build script was lacking a comma.".
  "</ul>".
  "So, nothing horribly broken. And plus, there are bugs in every release. That's why there's ".
  "always the next one. :) [Apologies to Isaac Richards]"
  );
madman_news_item("May 21, 2004",
  "Adam Ward made Slackware packages for madman. ".
  "Get them from the download area.");
madman_news_item("May 20, 2004",
  "It has finally happened! The madman source has finally reached a state that I ".
  "deem worthy of being called 0.93. Go grab it while it's hot!<p>".
  "The most massive changes since the last stable release (which was *gasp* 0.91.1) ".
  "include the addition of a very capable AutoDJ (Thanks, Shawn!) and a builtin web server, ".
  "minimization to the system tray, a new build system as well as over 100 other fixes ".
  "and improvements.<p/>".
  "I hope to have downloadable binary packages for a few distributions soon.");
madman_news_item( "Mar 28, 2004",
  "<b>Is madman randomly crashing on you?</b> The good news: It's not madman's fault. :) ".
  "The bad news: XMMS 1.2.9 has ".
  "a bug that causes programs controlling it to abort every once in a while. So, before ".
  "complaining, please upgrade to XMMS 1.2.10, which is available from ".
  "<a href=\"http://www.xmms.org\">the usual place</a>.<p>".
  "In other news, 0.93rc1 got quite a bunch of testing, Tim Dreessen deserves a big ".
  "thank you for all the bugs he found and reported. 0.93rc2 is now officially available ".
  "through the file release system.");
madman_news_item( "Mar 21, 2004",
  "Madman 0.93 release candidate 1 has finally happened, while my thesis is keeping me ".
  "busy... It's a pretty massive update, with lots of potential breakage. ".
  "Please give it a round of testing. Depending on the amount of actual ".
  "problems, 0.93 will be out very soon.<p>".
  "You can grab this version from CVS as tag MADMAN_RELEASE_0_93RC1 or from ".
  "the download page.<p>".
  "Major new features in this round: Rewritten AutoDJ (Shawn Willden's work), ".
  "Fixes for many string encoding bugs, an AutoTagger, passive popups and ".
  "a new build system.");
madman_news_item( "Dec 17, 2003",
  "madman now has an actual mailing list, with people on it and everything. ".
  "Wow. Little did I know. ;) Also, madman has an IRC channel to hang out on, ".
  "that is #madman on <a href=\"http://www.freenode.net/\">irc.freenode.net</a>. ".
  "I can be found lingering there every once in a while, everybody else is invited ".
  "to join. The next release, is, as always, late. Shawn and I are discussing ".
  "some fairly major changes to madman's core, but I still hope to release some ".
  "a working drop of the current code as 0.93beta1. Hold tight, it'll get done.".
  "I promise. ;)" );
madman_news_item( "Nov 23, 2003",
  "0.93 is still well underway. Shawn Willden agreed to rewrite the ".
  "AutoDJ, and Christian Plagemann made a new <a href=\"christian-madman-logo.png\">".
  "logo</a> for madman, which will be the official one from the next release. :) ".
  "As for myself, I'm down to three simple items on the ".
  "TODO list before 0.93 goes beta." );
madman_news_item( "Nov 11, 2003",
  "I just finished a pretty long coding session on madman, ".
  "and it just keeps getting better and better. ". 
  "Tonight, passive song announcement popups and tray icon support went in. ".
  "0.93beta1 will be out <i>real soon now</i>&trade;, there are only ".
  "a couple of things left to do. Preheat those compilers." );
madman_news_item( "Oct 26, 2003",
  "Oops. Release 0.92 will never happen. There are too many changes in ".
  "the code base already after the release candidate. ".
  "Instead, release 0.93beta1 ".
  "will be the next one out. 0.92rc1 seems to be fairly high-quality, ".
  "so I'd recommend using that for right now.<p>".
  "0.93 will have lots of new eye candy, will be <i>lots</i> ".
  "faster and plus, it'll be out soon." );
madman_news_item( "Sep 29, 2003",
  "Released version 0.92rc1. This is a source-only test ".
  "release for the next big version of madman, 0.92. It contains ".
  "many exciting new features like a web server, remote control and Auto DJ ".
  "as well as the usual slew of fixes, this time, among others, for ".
  "unicode cleanliness, UI and invalid characters.<p>".
  "The screenshots as well as all the rest of the site will be updated ".
  "for the full release.<p>".
  "Please help get this release tested before we put it out there. ".
  "<i>Update:</i> There's a small bug in 0.92rc1 that concerns writing Ogg ".
  "tags containing non-ASCII characters.");
madman_news_item( "Jun 28, 2003",
  "Released version 0.91.1 to fix a compilation issue regarding ".
  "overloaded <tt>setAttribute</tt> calls. Reported by Frank Baumgart.");
madman_news_item( "Jun 27, 2003",
  "Released version 0.91 with many new features. ".
  "Here's the freshmeat announcement:<p>".
  "This release adds fuzzy search, plugins (for CD burning), ".
  "a separate preferences dialog, and play history collection. ".
  "Each song now has a 'Performer' field as well as play statistics ".
  "and can be rated on a scale of 0 to 5 stars. Easy mass ".
  "tagging was implemented. A configurable number of backups will ".
  "be kept of a database. The build process now uses autotools. ".
  "Searches were made asynchronous for much faster response ".
  "times. Startup time was dramatically reduced." );
madman_news_item( "Feb 27, 2003",
  "Released patch version 0.90.1 to fix dependency on Qt 3.1.".
  "This new release only requires Qt 3.0 or higher." );
madman_news_item( "Feb 26, 2003",
  "Released initial version 0.90." );
?>
