<?
$madman_current_version="0.93";
$madman_development_version="0.93rc2";
$madman_deb_version="0.91-1";
function madman_header($pagetitle)
{
  echo "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n";
  echo "<html><head>\n";
  echo "<title>madman - $pagetitle</title>\n";
  echo "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n";
  echo "<link rel=\"stylesheet\" type=\"text/css\" href=\"format.css\">\n";
  echo "</head><body>\n";
}
function madman_footer()
{
  echo "</body></html>\n";
}
function madman_menu()
{
  echo "<div id=\"MadmanLogo\">";
  echo "<img src=\"madman_logo.png\" alt=\"madman logo\">";
  echo "</div>";
  echo "<div id=\"MadmanLettering\">";
  echo "<img src=\"madman_lettering.png\" alt=\"madman lettering\">";
  echo "</div>";
  echo "<div id=\"MadmanMenu\">";
  echo "<a href=\"index.php\">Home</a><br>";
  echo "<a href=\"download.php\">Download</a><br>";
  echo "<a href=\"tour1.php\">Tour</a><br>";
  echo "<a href=\"faq.php\">FAQ</a><br>";
  echo "<a href=\"screenshots.php\">Screenshots</a><br>";
  echo "<a href=\"http://sourceforge.net/projects/madman/\">Project page</a><br>";
  echo "<a href=\"http://sourceforge.net/mail/?group_id=59086\">Lists</a><br>";
  echo "<a href=\"http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/madman/madman2\">CVSWeb</a><p>";
  echo "<a href=\"http://sourceforge.net/donate/index.php?group_id=59086\"><img src=\"http://images.sourceforge.net/images/project-support.jpg\" width=\"88\" height=\"32\" border=\"0\" alt=\"Support This Project\" /> </a><p>";
  echo "<a href=\"http://sourceforge.net\"> ";
  echo "<img src=\"http://sourceforge.net/sflogo.php?group_id=59086&amp;type=1\" width=\"88\" height=\"31\" border=\"0\" alt=\"SourceForge Logo\"></a><p>";

  echo "<a href=\"http://validator.w3.org/check?uri=referer\">";
  echo "<img border=\"0\" src=\"http://www.w3.org/Icons/valid-html401\"";
  echo " alt=\"Valid HTML 4.01!\" height=\"31\" width=\"88\"></a></p>";

  echo "<a href=\"http://jigsaw.w3.org/css-validator/\">";
  echo "<img style=\"border:0;width:88px;height:31px\"";
  echo " src=\"http://jigsaw.w3.org/css-validator/images/vcss\" alt=\"Valid CSS!\"></a>";
  echo "</div>";
}
function madman_lastchanged( $change_date )
{
  echo "<div class=\"lastchanged\">Last changed by Andreas Kl&ouml;ckner";
  echo "&lt;inducer at users dot sourceforge dot net&gt; - $change_date</div><p>";
}
function madman_begin_main()
{
  echo "<div id=\"MadmanText\">";
}
function madman_end_main()
{
  echo "</div>";
}
$news_item_count = 0;
$more_news_items = 0;
function madman_news_item( $date, $news )
{
  global $news_item_count, $max_news_items, $more_news_items;
  $news_item_count++;
  if ( $news_item_count <= $max_news_items )
  {
    echo "<b>$date:</b><p>$news<p>";
  }
  else
  {
    $more_news_items = 1;
  }
}
function madman_shot( $caption, $name )
{
  $full_name = "shot_$name"."_full.png";
  $small_name = "shot_$name"."_small.png";
  echo "$caption<p><a href=\"$full_name\">";
  echo "<img alt=\"$caption\" src=\"$small_name\" class=\"ScreenShot\"></a><p>";
}
?>
