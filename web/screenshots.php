<? 
  include( "include.php" );
  madman_header("Main Page");
  madman_menu();
  madman_begin_main();

  madman_shot( "The main window", "main_window" );
  madman_shot( "The preferences window", "prefs_window" );
  madman_shot( "The preferences window (plugins tab)", "prefs_window_plugins" );
  madman_shot( "A sample context menu", "context_menu" );
  
  madman_lastchanged( "Tue Jun 17 18:48:13 CEST 2003" );
  madman_end_main();
  madman_footer();
?>


