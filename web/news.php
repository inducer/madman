<? 
  include( "include.php" );
  madman_header("News");
  madman_menu();
  madman_begin_main();

  $max_news_items = 1000;
  include( "news_items.php" );
    
  madman_lastchanged( "Wed Sep 24 20:39:06 CEST 2003" );
  madman_end_main();
  madman_footer();
?>
