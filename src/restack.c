void
restack_special (Display *d, Window root)
{
  Window rootr, parent, *children
  unsigned int nchildren;
  Window restack[1024];
  int nstackables;
  
  XQueryTree (d, root, &rootr, &parent, &children, &nchildren);
  /*do stuff*/
  /*
   * menu bar
   * stuff
   * desktop
   */
  XRestackWindows (d, restack, nstackables);
}
