% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%
% Main part.
%
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

% max command line length
/cmdlinelength 512 def

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%
% Install source is 64bit?
%
% ( ) ==> ( true|false )
%
/64bit_source {
  % 64 bit dir exists and is != 32 bit dir
  64bit_boot_dir 32bit_boot_dir ne
  64bit_boot_dir .undef ne and
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%
% Are we a dvd?
%
% ( ) ==> ( true|false )
%
/is_dvd {
  % check only once
  is_dvd.result .undef ne { is_dvd.result return } if

  /is_dvd.result
    mediatype m_cdrom eq {
      0x10 readsector
      dup 0x50 add getdword exch free
      9 shr % iso size in MB
      720 gt % assume dvd if > 720 MB
    } {
      false
    } ifelse
  def

  is_dvd.result return
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%
% Install source is 32 & 64bit?
%
% ( ) ==> ( true|false )
%
/32+64bit_source {
  32bit_boot_dir .undef ne
  % uncomment next line to automatically warn about 32bit software on 64bit machines, too
  % 64bit_boot_dir .undef ne and
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%
% Enough memory?
%
% ( ) ==> ( true|false )
%
/enough_mem {
  biosmem 4 20 shl lt
  biosmem mem.check 20 shl ge or
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%
% Check if there are boot directories for 32 & 64bit.
%
% Assumes 32bit to be in *i686* and 64bit in *x86_64*.
%
% ( ) ==> ( )
%
/check_arch_boot_dir {
  getcwd dup .undef ne {
    /64bit.tmp 256 string def

    dup "i686" strstr 0 ne over "x86_64" strstr 0 ne or {
      dup "i686" strstr {
        /32bit_boot_dir exch def
        /64bit_boot_dir 32bit_boot_dir "i686" "x86_64" strreplace
      } {
        /64bit_boot_dir exch def
        /32bit_boot_dir 64bit_boot_dir "x86_64" "i686" strreplace
      } ifelse

      dup "%s/isolinux.cfg" 64bit.tmp sprintf
      64bit.tmp filesize .undef ne { def } { free pop } ifelse

    } {
      /32bit_boot_dir over def
      /64bit_boot_dir exch def
    } ifelse

    64bit.tmp free

    % font.normal setfont
    % 0 400 moveto 32bit_boot_dir print
    % 0 420 moveto 64bit_boot_dir print dtrace

  } {
    pop
  } ifelse
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
%
% input event handling
%
% ( key ) ==> ( input_buffer menu_entry action )
%
% key
%  bit 0-7	ascii
%  bit 8-15	scan code
%  bit 16-32	status bits (ctrl, shift...)
%
% action
%    0:		ok, stay in input loop
%    1:		switch to text mode
%  >=2:		start linux
%
/KeyEvent {
  % timeout

  menu.texts 0 eq { pop "" -1 1 return } if

  dup 0 eq { boot.ed.list buildcmdline 2 return } if

  debug 4 ge {
    % print keycode somewhere
    -1 settransparentcolor
    white setcolor
    500 0 moveto dup print "  " print
  } if

  dup 0xff00 and 16 shl over 0xff and dup 0xe0 eq { pop 0 } if add /key exch def
  16 shr 0xffff and /keystat exch def

  key 24 shr 0xff and dup 0xf0 ge exch 0xf4 le and { key serial.input return } if

  key

  config.keymap { mapkey } if

  dup 0xffffff and dup { exch } if pop

  debug 4 ge {
    % print mapped key somewhere
    -1 settransparentcolor
    white setcolor
    500 20 moveto dup print "  " print
  } if

  % some special keys
  debug.input

  % put key through normal input queue
  window.input

  pop

  window.action actExit eq {
    /window.action actNothing def
    "" -1 1 return
  } if

  window.action actRestore eq {
    /window.action actNothing def
    "" config.restore.id 2 return
  } if

  window.action actCloseInfo eq {
    /window.action actNothing def
    % cd change dialog
    info.type 5 eq info.type 6 eq or {
      % 'mount' re-reads fs metadata
      mount pop
    } if

    "" -1 3 return
  } if

  window.action actPassword eq {
    /window.action actNothing def
    password.dialog {
      password.dialog .ed.buffer.list get 0 get
    } { "" } ifelse
    -1 3 return
  } if

  window.action actStart eq {
    /window.action actNothing def
    /load_error false def

    config.memcheck {
      menu.texts menu.entry 0 max menu.texts length 1 sub min get
      dup "mem.min[%s]" mem.min sprintf
      "mem.msg[%s]" mem.msg sprintf

      enough_mem not {
        nomem_popup
        "" -1 0 return
      } if
    } if

    % some tricks to make it possible to review the complete command line
    debug 3 ge xxx.cmdline .undef eq and {
      /window.action actNothing def

      boot.ed.list buildcmdline /xxx.menu exch def /xxx.cmdline exch def

      /dia window.dialog def
      dia .title "Kernel command line" put
      dia .text "" put
      % Must all be of same size!
      dia .ed.list 1 array put
      dia .ed.buffer.list [ xxx.cmdline ] put
      dia .ed.text.list [ "All options\n" ] put

      dia .ed.focus 0 put
      dia .ed.width 600 put
      dia .ed.font font.normal put
      dia .buttons [
        button.ok button.default actStart button.setaction
        button.cancel button.notdefault /xxx.cancel button.setaction
        config.rtl { exch } if
      ] put
      dia window.init
      dia window.show

      "" -1 0
    } {
      sound.done

      xxx.cmdline .undef ne {
        xxx.cmdline xxx.menu
        /xxx.cmdline .undef def
      } {
        boot.ed.list buildcmdline
      } ifelse
      2 return
    } ifelse
  } if

  window.action actRedraw eq {
    /window.action actNothing def
    main.redraw
  } if

  window.action actRedrawPanel eq {
    /window.action actNothing def
    panel.show
  } if

  "" -1 0
} def


/xxx.cancel {
  /xxx.cmdline .undef def
  actNothing
} def


/bc.cmd cmdlinelength string def
/mem.min 128 string def
/mem.msg 256 string def


% ( edit_object_list ) ==> ( cmdline menu_entry )
%
/buildcmdline {
  menu.entry 0 lt menu.entry menu.texts length ge or { pop "" -1 return } if

  [ over
    {
      dup .inp_show get {
        build1cmdline strdup
      } {
        pop
      } ifelse
    } forall

    % empty? -> pass at least the first edit object
    dup [ eq { over 0 get build1cmdline strdup } if

  ]

  dup boot.splitstr join bc.cmd over strcpy pop free
  dup { free } forall free

  pop bc.cmd menu.entry

  % increment menu idx to account for removed entry
  config.restore.id .undef ne {
    menu.entry config.restore.id ge { 1 add } if
  } if

} def


% ( edit_object ) ==> ( cmdline )
%
% grub:
%   just return
%
% syslinux & lilo:
%   add kernel name at start of command line
%
/build1cmdline {
  syslinux {
    % set new working directory
    xmenu.bits .xm_current get 1 eq { 64bit_boot_dir } { 32bit_boot_dir } ifelse
    dup .undef ne {
      dup getcwd ne { chdir } { pop } ifelse
    } {
      pop
    } ifelse
  } if
  % getcwd 0 300 moveto show trace

  % lilo & syslinux: prepend kernel/label name
  grub {
    bc.cmd 0 0 put
  } {
    menu.texts menu.entry get "%s " bc.cmd sprintf
  } ifelse

  config.spl {
    xmenu.video {
      /splash_file 32 string def

      video.modes.list xmenu.video .xm_current get get
      dup .vm_mode get 0 ge {
        ",19201200.spl" splash_file sprintf
      } if
      pop

      % check whether splash file exists
      splash_file 1 add filesize .undef eq {
        splash_file 0 0 put	% empty string
      } if
    } if
  } if

  xmenu.kernelopts {
    kernelopts.options xmenu.kernelopts .xm_current get get dup "" ne {
      "%s " bc.cmd dup length add sprintf
    } { pop } ifelse
  } if

  /cmdline.hidden over dup length .inp_hidden ge { .inp_hidden get } { pop .undef } ifelse def

  cmdline.hidden {
    cmdline.hidden "" ne { 
      splash_file {
        /bc.tmp cmdline.hidden length splash_file length add string def
        cmdline.hidden "initrd=" bootopt.find dup .undef ne {
          skipnonspaces
          dup
          dup 0 get over 0 0 put
          splash_file cmdline.hidden "%s%s" bc.tmp sprintf
          0 exch put
          "%s" bc.tmp dup length add sprintf
        } {
          pop
          bc.tmp cmdline.hidden strcpy pop
        } ifelse

        bc.tmp "%s " bc.cmd dup length add sprintf

        bc.tmp free /bc.tmp .undef def
      } {
        cmdline.hidden "%s " bc.cmd dup length add sprintf
      } ifelse
    } if
  } if

  /splash_file xfree

  syslinux {
    video.modes.list xmenu.video .xm_current get get .vm_options get
    "%s " bc.cmd dup length add sprintf

    v_impaired 1 ge {
      "braille=1 " bc.cmd dup length add sprintf
    } if

    v_impaired 2 ge {
      "linemode=1 " bc.cmd dup length add sprintf
    } if
  } if

  config.addopt.lang {
    config.lang {
      config.lang "lang=%s " bc.cmd dup length add sprintf
    } if
  } if

  config.addopt.keytable {
    config.keymap.id {
      config.keymap.id "keytable=%s " bc.cmd dup length add sprintf
    } if
  } if
  
  config.addopt.layout {
    config.layout {
      config.layout "layout=%s " bc.cmd dup length add sprintf
    } if
  } if
  
  xmenu.profile {
    profile.options xmenu.profile .xm_current get get dup "" ne {
      "%s " bc.cmd dup length add sprintf
    } { pop } ifelse
  } if

  % add user-supplied options

  bc.cmd exch .inp_buf get strcat " " strcat pop

  % remove all but last 'vga' & 'initrd' options
  [ "vga" "initrd" ] dup {
    {
      bc.cmd over bootopt.find2 .undef eq { pop exit } if
      bc.cmd over bootopt.remove free
    } loop
  } forall free

  bc.cmd "driverupdate" bootopt.find dup .undef ne {
    % exclude 'driverupdate=0 and driverupdate=1'
    "driverupdate=" length get
    dup '0' ne exch '1' ne and {

      /bc.tmp bc.cmd "driverupdate" bootopt.remove def
      bc.tmp .undef ne {
        /bc.tmp bc.tmp "driverupdate=" length add ',' split bc.tmp free def
        /bc.tmp2 cmdlinelength string def

        bc.tmp2 "initrd=" strcpy pop

        bc.tmp length 0 gt {
          % add '+' to first arg unless it starts with '+' or '-'
          bc.tmp 0 get 0 get
          dup '-' eq {	% remove '-'
            bc.tmp 0 get dup 1 add strdup exch free
            bc.tmp exch 0 exch put
            pop '+'
          } if
          '+' ne {		% add '+'
            bc.tmp 0 get length 1 add string "+" strcat bc.tmp 0 get strcat
            bc.tmp 0 get free bc.tmp exch 0 exch put
          } if

          bc.tmp length 1 sub -1 0 {
            bc.tmp exch get
            bc.tmp2 exch strcat "," strcat pop
          } for
        } if

        bc.tmp dup { free } forall free

        % last 'initrd' option
        /bc.tmp .undef def
        {
          bc.cmd "initrd" bootopt.remove dup .undef ne {
            bc.tmp free /bc.tmp exch def
          } {
            pop exit
          } ifelse
        } loop

        % re-add new 'initrd' option
        bc.tmp {
          bc.tmp2 bc.tmp "initrd=" length add strcat pop
          bc.tmp free
          bc.cmd bc.tmp2 strcat " " strcat pop
        } if
      } if

    } if
  } { pop } ifelse

  syslinux {
    % find out initrd sizes for kernel loading progress bar
    /progress_extra 0 def
    bc.cmd "initrd" bootopt.find dup .undef ne {
      "initrd=" length add
      dup dup skipnonspaces
      sub neg
      string
      exch strcpy
      % it's a ',' separated list
      dup ',' split exch free
      dup
      {
        dup
        % skip leading '+' or '-'
        dup 0 get dup '+' eq exch '-' eq or { 1 add } if
        filesize dup .undef ne {
          dup -2 eq { /progress_nosize true def } if
          sectorsize 1 sub add
          sectorsize div /progress_extra exch progress_extra add def
        } { pop } ifelse
        free
      } forall
      free
    } { pop } ifelse
  } if

  % remove final space
  bc.cmd dropspaces

  bc.cmd
} def


/iso.needscheck {
  false

  /i_tmp 0x10 readsector def
  i_tmp {
    i_tmp dup length 1 sub 0 put
    /i_tmp2 i_tmp 0x373 add cvs def
    i_tmp2 "check=1" strstr {
      pop true
    } if
    
    i_tmp free
  } if
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( menu_entries_array cmdline_args_array defaultentry ) == > ( )
/MenuInit {
  bsplash.done

  colorbits 8 le {
    0 setcolor 0 0 moveto screen.size fillrect loadpalette
  } if

  init

  /menu.entry -1 def

  /menu.dentry exch def
  /menu.args exch def
  /menu.texts exch def

  menu.texts length 0 eq { return } if

  /mi_name "mediacheck" "" gfxconfig.set.str

  mi_name {
    /remove.mediachk false def
    sectorsize 0x800 eq {
      /remove.mediachk iso.needscheck not def
    } if
  } {
    /remove.mediachk false def
  } ifelse

  config.restore.title {
    /mi_name config.restore.title def
    /remove.mediachk true def

    0
    menu.texts {
      config.restore.title eq { /config.restore.id over def exit } if
      1 add
    } forall
    pop
  } if

  remove.mediachk menu.dentry mi_name ne and {
    /mi_tmp 0 def
    menu.texts {
      mi_name eq { exit } if
      /mi_tmp inc
    } forall

    mi_tmp menu.texts length lt {
      /mi_tmp2 0 def
      /menu.texts
      [
        menu.texts { mi_tmp2 mi_tmp eq { pop } if /mi_tmp2 inc } forall
      ]
      def

      /mi_tmp2 0 def
      /menu.args
      [
        menu.args { mi_tmp2 mi_tmp eq { pop } if /mi_tmp2 inc } forall
      ]
      def
    } if
  } if

  window.main
  dup window.init
      window.show

  config.beep { 3000 50000 beep } if
  
  % lang.displayed not { panel.lang } if

  syslinux {
    32+64bit_source not {

      64bit {
        64bit_source not { notimeout 32bit_popup } if
      } {
        64bit_source { notimeout 64bit_popup } if
      } ifelse

    } if
  } if

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( text errorcode ) ==> ( )
% errorcode:
%   0	normal info
%   1	fatal error
%   2	missing file
%   3   disk change
%   4   disk change failed
%   5   ask for cd change
%   6   change cd back to original
%

/info.tmpmsg 256 string def

/InfoBoxInit {
  /info.type exch def
  /info.msg exch def

  window.dialog

  info.type 1 eq {
    dup .title.bg window.title.error.bg put
    dup .title txt_error_title put
    dup .buttons
    [
      button.reboot button.default actCloseInfo button.setaction
    ] put
  } {
    dup .title txt_info_title put
    dup .buttons
    [
      button.ok button.default actCloseInfo button.setaction
%      button.cancel button.notdefault actCloseInfo button.setaction
    ] put
  } ifelse

  syslinux info.type 2 eq and {
    dup .title "I/O Error" put
    info.msg "File not found: %s" info.tmpmsg sprintf
    /info.msg info.tmpmsg def
    /load_error true def
  } if

  syslinux info.type 3 eq and {
    dup .title txt_change_disk_title put
    0 getinfo 1 add txt_insert_disk info.tmpmsg sprintf
    /info.msg info.tmpmsg def
  } if

  syslinux info.type 4 eq and {
    dup .title txt_change_disk_title put
    1 getinfo 15 not and {
      0 getinfo 1 add
      txt_insert_disk3 info.tmpmsg sprintf
    } {
      0 getinfo 1 add 1 getinfo 1 add
      txt_insert_disk2 info.tmpmsg sprintf
    } ifelse
    /info.msg info.tmpmsg def
  } if

  syslinux info.type 5 eq and {
    dup .title "Driver Update" put
    info.msg "Insert driver update CD-ROM for\n\"%s\"." info.tmpmsg sprintf
    /info.msg info.tmpmsg def
  } if

  syslinux info.type 6 eq and {
    dup .title "Driver Update" put
    /info.msg "Put the MyArch media back into the drive." def
  } if

  dup .text info.msg put

  serial.line.status {
    serial.infobox .undef eq {
      /serial.infobox .xm_size array def
      serial.infobox .xm_current 0 put
    } if

    serial.infobox .xm_title 2 index .title get put
    serial.infobox .xm_text info.msg put
    serial.infobox .xm_list [ info.type 1 eq { "reboot" } { "ok" } ifelse ] put

    serial.infobox.setup
  } if

  dup window.init
      window.show

} def



% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

% progress bar code


% Show percentage of progress bar.
%
% ( percentage ) ==> ( )
%
/progress.percent {
  0 max 100 min		% so people don't ask silly questions...
  "100%" strsize over neg progress.text.x add progress.text.y moveto
  window.current .color.bg get setcolor
  fillrect  

  "%3u%%" 8 string dup 4 1 roll sprintf

  dup strsize pop neg progress.text.x add progress.text.y moveto
  window.current .color.fg get setcolor

  serial.line.status {
    "\x08\x08\x08\x08" serial.show
    dup serial.show
  } if

  dup show
  free

} def


% Show n-th progress bar symbol.
%
% ( n ) ==> ( )
%
/progress.sym.show {
  /progress.sym.current exch def

  progress.bar.x progress.bar.y moveto
  progress.sym.width progress.sym.current 1 sub mul 1 add 1 rmoveto
  progress.sym.width 2 sub
  progress.bar.height 2 sub
  progress.bar.color setcolor
  fillrect

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( kernel_name ) ==> ( )
/ProgressInit {
  /progress.kname exch def

  boot.ed.list { edit.hidecursor } forall

  /dia window.dialog def

  dia .width.min 350 put
  dia .position 10 put

  dia .title txt_load_kernel_title put
  dia .text
    progress.kname "memtest" eq {
      txt_load_memtest
    } {
      txt_load_kernel
    } ifelse
  put

  dia window.init
  dia window.show

  % now add progress bar

  dia .x get dia .y get moveto
  dia .text.x get dia .text.y get 28 add rmoveto

  /progress.bar.height 19 def
  /progress.bar.width dia .width get progress_nosize { 25 } { 75 } ifelse sub def

  /progress.sym.width 10 def
  /progress.bar.width
    progress.bar.width progress.sym.width div
    /progress.syms over def progress.sym.width mul
  def

  currentpoint over 1 sub over 2 sub moveto
  black white progress.bar.width 2 add progress.bar.height 4 add drawborder

  /progress.bar.y exch def
  /progress.bar.x exch def

  /progress.text.x progress.bar.x progress.bar.width 55 add add def
  /progress.text.y progress.bar.y progress.bar.height fontheight sub 2 div add def

  /progress.sym.current 0 def

  progress_nosize not { 0 progress.percent } if

  serial.line.status {
    "\n" serial.show
    dia .text get serial.show
    "  0%" serial.show
  } if
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( ) ==> ( )
/ProgressDone {
  window.done

  load_error {
    /load_error false def
    boot.ed.list boot.window .ed.focus get get edit.showcursor
  } if

  serial.line.status { "\n\n" serial.show } if

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( max current ) ==> ( )
%
% Note: max is just kernel size (in sectors).
%
/ProgressUpdate {
  progress_nosize {
    exch pop
    sectorsize mul 10 shr
    progress.bar.x progress.bar.y moveto
    window.color.bg setcolor
    progress.bar.width progress.bar.height fillrect
    progress.bar.width 10 sub 0 rmoveto
    window.color.fg setcolor
    64 string exch
    "%d kB" 2 index sprintf
    dup showright free

    return
  } if

  exch progress_extra add exch

  over over 100 mul exch 1 max div progress.percent

  progress.syms mul progress.syms 2 div add exch 1 max div

  0 max progress.syms min

  dup progress.sym.current gt {
    progress.sym.current 1 add over 1 exch {
      progress.sym.show
    } for
  } if
  pop

} def

% initrd size (in sectors)
/progress_extra 0 def

% for pxe we don't get file sizes
/progress_nosize false def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( time ) ==> ( )
% /Timer { pop } def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( label correct_password ) ==> ( )
%
/PasswordInit {
  /password.key exch def pop

  /dia window.dialog def

  /password.dialog dia def

  dia .title txt_password_title put
  dia .text "" put

  % Must all be of same size!
  dia .ed.list 1 array put
  dia .ed.buffer.list [ 31 string ] put
  dia .ed.text.list [ txt_password ] put

  dia .ed.focus 0 put

  dia .ed.width 200 put

  dia .ed.font font.normal pwmode put

  dia .buttons
    [ button.ok button.default actPassword button.setaction ]
  put

  dia window.init
  dia window.show

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( password ) ==> ( error )
%
% error:
%   true   password ok
%   false  wrong password
%
% ****** FIXME: test result seems to be unused
%
/PasswordDone {

  password.key eq
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( text ) == > ( new_text )
/menuitemmap {
  translate
  config._2space {
    /mmm.tmp xfree
    strdup /mmm.tmp over def
    dup length 0 gt {
      0 1 2 index length 1 sub {
        over over get '_' eq { over exch ' ' put } { pop } ifelse
      } for
    } if
  } if
  dup "memtest" eq over "memtest86" eq or { pop txt_memtest return } if
  dup "firmware" eq { pop txt_firmware return } if
  mode.install {
    dup "start"          eq { pop txt_start          return } if
    dup "nonfree"        eq { pop txt_nonfree        return } if
    dup "hdt"            eq { pop txt_hdt            return } if
    dup "linux"          eq { pop txt_install        return } if
    dup "harddisk"       eq { pop txt_boot_harddisk  return } if
  } {
    dup "failsafe"       eq { pop txt_safe_linux     return } if
    dup "linux"          eq { pop "Linux"            return } if
    dup "windows"        eq { pop "Windows"          return } if
  } ifelse
  % still no luck... - try to translate
  _
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( color0 color1 width height ) ==> ( )
/drawborder {
  currentpoint /db.y0 exch def /db.x0 exch def

  /db.y1 exch 1 sub db.y0 add def
  /db.x1 exch 1 sub db.x0 add def
  /db.col1 exch def
  /db.col0 exch def

  db.x0 db.y1 moveto

  db.col0 setcolor
  db.x0 db.y0 lineto db.x1 db.y0 lineto

  db.col1 setcolor
  db.x1 db.y1 lineto db.x0 db.y1 lineto
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( color0 color1 color2 width height ) ==> ( )
% draw frame with shadow
% color0: upper left, color1: lower right, color2: shadow
/drawborder3 {
  currentpoint /db.y0 exch def /db.x0 exch def

  /db.y1 exch 1 sub db.y0 add def
  /db.x1 exch 1 sub db.x0 add def
  /db.col2 exch def
  /db.col1 exch def
  /db.col0 exch def

  db.x0 db.y1 moveto

  db.col0 setcolor
  db.x0 db.y0 lineto db.x1 db.y0 lineto

  db.col1 setcolor
  db.x1 db.y1 lineto db.x0 db.y1 lineto

  db.col2 -1 ne {
    db.col2 setcolor
    1 1 rmoveto
    db.x1 1 add db.y1 1 add lineto
    db.x1 1 add db.y0 1 add lineto
  } if
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% ( color0 color1 width height ) ==> ( )
/drawborder4 {
  3 index 4 1 roll rot dup 3 index 3 index
  currentpoint 6 2 roll
  1 1 rmoveto drawborder
  moveto drawborder
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% center text
% ( text width height ) ==> ( )
/centertext {
  3 -1 roll strsize
  4 2 roll
  4 1 roll exch 4 1 roll sub 2 div neg 3 1 roll sub 2 div neg
} def


% ( img ) => ( width heigh )
/imgsize {
  dup .undef eq {
    pop 0 0
  } {
    getdword dup
    0xffff and exch 16 shr
  } ifelse
} def


% ( pic pic_a -- )
/showlogo {
  /tmp.sc over imgsize savescreen def

  currentpoint 4 2 roll
  0 0 moveto tmp.sc blend
  moveto tmp.sc restorescreen

  tmp.sc free
} def


% ( file_name ) ==> ( image )
/readimage {
  findfile dup .undef ne {
    dup setimage 0 0 image.size unpackimage exch free
  } if
} def


/init {
  0 0 moveto currentpoint clip.size image

  % set default language
  "lang" findfile dup {
    /tmp over length 1 add 2 max string def
    tmp exch {
      dup ' ' eq over '\n' eq or { pop pop exit } if
      over exch 0 exch put
      1 add
    } forall
    tmp dup setlang pop getkeymapfromlocale setkeymap
    % /lang.displayed true def
  } {
    pop
    "en_US" dup setlang pop getkeymapfromlocale setkeymap
    % reset, it's just the fallback lang
    /config.lang.changed false def
  } ifelse

  keymap.default "" ne { keymap.default setkeymap } if

  font.large setfont

  /config.layout config.lang getlayoutfromlocale def
  
  /menu.text.xofs      10 def
  /menu.text.yofs       2 def
  /menu.item.height    fontheight dup 2 div add def
  /menu.bar.height     fontheight menu.text.yofs dup add add def

  font.normal setfont

  /menu.bar.width menu.bar.min.width def

  % false: no boot options line initially
  /boot.show true def

} def


/chksum {
  0 exch
  { add } forall
} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Some special debug & test keys.
%
% ( key_in ) ==> ( key_out )
%
/debug.input {
  dup keyShiftF4 eq debug 3 ge and {
    currentcolor black setcolor
    currentpoint 0 0 moveto
    64 string biosmem "mem = %u  " 2 index sprintf dup show free
    moveto setcolor
  } if

  dup keyShiftF3 eq syslinux and debug 3 ge and {
    currentcolor black setcolor
    currentpoint 0 0 moveto
    "eject  " print bootdrive eject print
    moveto setcolor
  } if

  dup keyShiftF5 eq syslinux and debug 3 ge and {
    currentcolor black setcolor
    currentpoint 100 0 moveto
    bootdrive print
    moveto setcolor
  } if

  dup keyShiftF8 eq debug 3 ge and {
    currentcolor debug 1 and { white } { black } ifelse setcolor

    currentpoint 300 0 moveto
    0 memsize print "/" print print "  " print
    moveto

    currentpoint 300 20 moveto
    1 memsize print "/" print print "  " print
    moveto

    currentpoint 300 40 moveto
    2 memsize print "/" print print "  " print
    moveto

    currentpoint 300 60 moveto
    3 memsize print "/" print print "  " print
    moveto

    currentpoint 300 80 moveto
    4 memsize print "/" print print "  " print
    moveto

    setcolor
  } if

  dup keyShiftF9 eq debug 3 ge and {
    dumpmem
  } if

  dup keyShiftF10 eq {
    /debug debug 1 add def
  } if

  dup keyShiftF4 eq {
    % sound test XXXXXXXX

    sound.done

  } if

  dup keyF11 eq debug 1 ge and {

    /dit {
      dup 0xff and rand 0xff and gt { 0xff add } if
      8 shr
    } def

    /c13.r rand 0x7f00 and def
    /c02_13.r rand 0x7f00 and c13.r sub def
    /c23.r rand 0x7f00 and def
    /c01_23.r rand 0x7f00 and c23.r sub def
    /c13.g rand 0x7f00 and def
    /c02_13.g rand 0x7f00 and c13.g sub def
    /c23.g rand 0x7f00 and def
    /c01_23.g rand 0x7f00 and c23.g sub def
    /c13.b rand 0x7f00 and def
    /c02_13.b rand 0x7f00 and c13.b sub def
    /c23.b rand 0x7f00 and def
    /c01_23.b rand 0x7f00 and c23.b sub def

    screen.size /h exch def /w exch def

    0 1 screen.size exch pop {
      0 1 screen.size pop {
        over moveto

        currentpoint c01_23.r mul h div exch c02_13.r mul w div add c13.r add c23.r add
        dit 0 max 0xff min 16 shl
        currentpoint c01_23.g mul h div exch c02_13.g mul w div add c13.g add c23.g add
        dit 0 max 0xff min  8 shl
        currentpoint c01_23.b mul h div exch c02_13.b mul w div add c13.b add c23.b add
        dit 0 max 0xff min add add

        setcolor putpixel
      } for
      pop
    } for

    pop 0
  } if

  dup keyF12 eq debug 6 ge and {
    % mouse support testing

    1 keepmode

    /mouse xxx def
    0xc00000 setcolor
    mouse .undef eq {
      700 580 moveto "no mouse" show
    } {
      {
        mouse getdword 16 shl 16 shr screen.size pop 2 div add
        mouse getdword 16 shr neg screen.size exch pop 2 div add
        moveto 4 4 fillrect

        mouse 4 add getdword 7 and
        dup 1 and { 0xc00000 setcolor } if
        dup 2 and { 0x0000a0 setcolor } if
        dup 4 and { 0x009000 setcolor } if

        3 and 3 eq { exit } if

      } loop
    } ifelse
    pop 0
  } if

  dup keyShiftF12 eq {
    /transp transp 0x10 add 0x100 min def
    pop 0
  } if

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Show exit popup.
%
% ( ) ==> ( )
%
/exit_popup {
  window.dialog

  dup .title txt_exit_title put
  dup .text txt_exit_dialog put
  dup .buttons [
    button.ok button.default actExit button.setaction
    button.cancel button.notdefault actNothing button.setaction
    config.rtl { exch } if
  ] put
  dup window.init
      window.show

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Show help window.
%
% ( ) ==> ( )
%
/show_help {
  window.help
  
  dup window.init
  window.show

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Warn that we are about to install 32bit software on a 64bit system.
%
% ( ) ==> ( )
%
/32bit_popup {
  window.dialog

  dup .title "Cool computer, but..." put
  dup .text "You are about to install 32-bit software on a 64-bit computer." put
  dup .buttons [
    button.continue button.default actNothing button.setaction
  ] put
  dup window.init
      window.show

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Warn that we are about to install 64bit software on a 32bit system.
%
% ( ) ==> ( )
%
/64bit_popup {
  window.dialog

  dup .title "Cool software, but..." put
  dup .text "This is a 32-bit computer. You cannot use 64-bit software on it." put
  dup .buttons [
    button.reboot button.default actReboot actNoClose or button.setaction
  ] put
  dup window.init
      window.show

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Warn that we don't have enough memory.
%
% ( ) ==> ( )
%
/nomem_popup {
  window.dialog

  dup .title "Sorry, but..." put
  dup .text mem.show put
  dup .buttons [
    button.ok button.default actNothing button.setaction
  ] put
  dup window.init
      window.show

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Show "power off" popup.
%
% ( ) ==> ( )
%
/power_off {
  window.dialog

  dup .title txt_power_off_title put
  dup .text txt_power_off put
  dup .buttons [
    button.ok     button.notdefault    actPowerOff actNoClose or button.setaction
    button.cancel button.default       actNothing                button.setaction
    config.rtl { exch } if
  ] put
  dup window.init
      window.show

} def


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Set global config variables.
%

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% For monitor tests: fake DDC info.
%

% "ddc" findfile test1


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% First, figure out video mode.
%

/config.screen.width /config.screen.height "screen.size" 800 600 gfxconfig.set.int2

/screen.fallback false def

config.screen.width config.screen.height 32 findmode setmode not {
  config.screen.width config.screen.height 16 findmode setmode not {
    /screen.fallback true def
    640 480 16 findmode setmode not {
      640 480 32 findmode setmode not {
        false .end
      } if
    } if
  } if
} if

% prepend fallback section to layout section list
screen.fallback {
  /gfxconfig.layout [ "640x480" gfxconfig.layout { } forall ] gfxconfig.layout free def
} if

% for compatibility: prepend live section to layout section list if 'livecd' is set
/config.livecd         "livecd"                   false gfxconfig.set.bool
config.livecd {
  /gfxconfig.layout [ "live" gfxconfig.layout { } forall ] gfxconfig.layout free def
} if


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
% Then, read the other config variables (might depend on video mode).
%

/config.welcome        "welcome"                      0 gfxconfig.set.int
/config.beep           "beep"                     false gfxconfig.set.bool
/config.volume         "volume"                      70 gfxconfig.set.int
/config.talk           "talk"                     false gfxconfig.set.bool
/config._2space        "_2space"                  false gfxconfig.set.bool
/config.memcheck       "memcheck"                 false gfxconfig.set.bool
/config.spl            "spl"                      false gfxconfig.set.bool
/config.autodown       "autodown"                 false gfxconfig.set.bool
/transp                "menu.transparency"         0x20 gfxconfig.set.int
/config.screen.width
/config.screen.height  "screen.size"            800 600 gfxconfig.set.int2
/config.background     "background"          "back.jpg" gfxconfig.set.str
/config.font.normal    "font.normal"        "16x16.fnt" gfxconfig.set.str
/config.font.large     "font.large"         "16x16.fnt" gfxconfig.set.str
/config.restore.title  "restore.title"               "" gfxconfig.set.str
/progress.bar.color    "progress.bar.color"       white gfxconfig.set.int
/xmenu.normal.bg       "menu.normal.bg"           black gfxconfig.set.int
/xmenu.normal.fg       "menu.normal.fg"           white gfxconfig.set.int
/xmenu.normal.alt.fg   "menu.normal.alt.fg"    0x606060 gfxconfig.set.int
/xmenu.selected.bg     "menu.selected.bg"         white gfxconfig.set.int
/xmenu.selected.fg     "menu.selected.fg"         black gfxconfig.set.int
/window.color.bg       "window.bg"                white gfxconfig.set.int
/window.color.fg       "window.fg"                black gfxconfig.set.int
/window.title.bg       "window.title.bg"          black gfxconfig.set.int
/window.title.fg       "window.title.fg"          white gfxconfig.set.int
/window.title.error.bg "window.title.error.bg" 0xc00000 gfxconfig.set.int
/help.link.fg          "help.link.fg"          0x0000a0 gfxconfig.set.int
/help.link.selected.fg "help.link.selected.fg"    white gfxconfig.set.int
/help.link.selected.bg "help.link.selected.bg" 0x0000a0 gfxconfig.set.int
/help.highlight.fg     "help.highlight.fg"     0x009000 gfxconfig.set.int
/menu.text.normal      "mainmenu.normal.fg"       white gfxconfig.set.int
/menu.text.select      "mainmenu.selected.fg"     black gfxconfig.set.int
/boot.text.options     "bootopt.label.fg"         white gfxconfig.set.int
/boot.text.normal      "bootopt.text.fg"          white gfxconfig.set.int
/menu.bar.color        "mainmenu.bar.color"       white gfxconfig.set.int
/menu.bar.min.width    "mainmenu.bar.minwidth"      300 gfxconfig.set.int
/menu.bar.transparency "mainmenu.bar.transparency"   70 gfxconfig.set.int
/menu.start.x
/menu.start.y          "mainmenu.pos"           253 170 gfxconfig.set.int2
/menu.max.entries      "mainmenu.entries"             8 gfxconfig.set.int
/boot.pos.x
/boot.pos.y            "bootopt.pos"            263 480 gfxconfig.set.int2
/panel.normal.fg       "panel.normal.fg"          white gfxconfig.set.int
/panel.title.fg        "panel.title.fg"           white gfxconfig.set.int
/panel.f-key.fg        "panel.f-key.fg"           black gfxconfig.set.int
/keymap.submenu        "keymap.submenu"           false gfxconfig.set.bool
/keymap.default        "keymap"                      "" gfxconfig.set.str

/config.addopt.lang    "addopt.lang"              false gfxconfig.set.bool
/config.addopt.keytable "addopt.keytable"         false gfxconfig.set.bool
/config.addopt.layout  "addopt.layout"            false gfxconfig.set.bool

/config.key.F2         "key.F2"                      "" gfxconfig.set.str
/config.key.F3         "key.F3"                      "" gfxconfig.set.str
/config.key.F4         "key.F4"                      "" gfxconfig.set.str
/config.key.F5         "key.F5"                      "" gfxconfig.set.str
/config.key.F6         "key.F6"                      "" gfxconfig.set.str
/config.key.F7         "key.F7"                      "" gfxconfig.set.str
/config.key.F8         "key.F8"                      "" gfxconfig.set.str
/config.key.F9         "key.F9"                      "" gfxconfig.set.str
/config.key.F10        "key.F10"                     "" gfxconfig.set.str
/config.key.F11        "key.F11"                     "" gfxconfig.set.str
/config.key.F12        "key.F12"                     "" gfxconfig.set.str

/config.product        "product"             "MyArch" gfxconfig.set.str

/config.init           "init"                        "" gfxconfig.set.str

/serial.line0          "serial.line0"               [ ] gfxconfig.set.array_str
/serial.line1          "serial.line1"               [ ] gfxconfig.set.array_str
/serial.line2          "serial.line2"               [ ] gfxconfig.set.array_str
/serial.line3          "serial.line3"               [ ] gfxconfig.set.array_str

/serial.lines [
  serial.line0 serial.line1 serial.line2 serial.line3
] def

% Array of menu entries with no boot option input field.
/config.nobootoptions "nobootoptions"       [ ] gfxconfig.set.array_str

% Array of menu entries which should not get an 'install' option appended.
/config.noinstallopt  "noinstallopt"        [ ] gfxconfig.set.array_str

% Mininum required memory in MB.
/mem.check {
  mem.min gfxconfig.int dup .undef eq { pop 0 } if
} def

% Message to print if we fail minimum memory requirement.
/mem.show {
  mem.msg gfxconfig.str dup .undef eq { pop "Not enough memory." } if
} def

/mode.install false def
gfxconfig.layout { "install" eq { /mode.install true def exit } if } forall

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

% drawing area size
/clip.size { screen.size } def

% set background image
config.background findfile setimage

% get font(s)
/font.normal config.font.normal findfile def
/font.large
  % don't load it twice
  config.font.normal config.font.large eq {
    font.normal
  } {
    config.font.large findfile
  } ifelse
def

% default debug level
/debug 0 def

% no specials for visual impaired people
/v_impaired 0 def

% show welcome animation?
screen.fallback
CapsLock or
config.welcome 0 eq or
{
  /bsplash.done { } def
} {
  bsplash.show
} ifelse

% check for bi-arch config
check_arch_boot_dir
