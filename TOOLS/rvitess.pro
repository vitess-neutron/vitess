;Program to read in the output from writeout modules of Vitess
;Written by J.V. Pearce, Pennsylvania State University, 25 February 2002
;Get routine written by D.G. Narehood, Pennsylvania State University
;screenread and printing routines written by Liam Gumley

; Adopted to Vitess 2.4 in April 2003, by Michael Fromme, HMI
; goals were:
; - correct input from vitess output files
;   read binary output files from Vitess (stay to the same computer
;   platform where you use Vitess!)
; - support for PV-Wave environments
;   common rvitess.pro, special rvit_wave.pro and rvit_idl.pro
;
; Call this routine from the IDL or WAVE prompt with
;   cd, <tooldir>
;   rvitess
; where <tooldir> is the Vitess tool directory like
; C:\Programs\Vitess\Tools   for Windows.
; You may also copy rvit*.pro to your favorite
; environment and omit the cd, .. command.


PRO READ_DATA, finalaction
  COMMON MAINOPTS, slash, wdevice, pdevice, isidl
  COMMON MCommon, file,user_operation
  COMMON SHARE1,a,b,c,d,e,f,g,h,m,p,q,r,s

  ; ON_IOERROR, leave
  ; ON_ERROR, 2

  IF N_ELEMENTS(finalaction) LE 0 THEN finalaction = ''
  getFilename, finalaction
  IF finalaction NE ''  OR file EQ '' THEN RETURN

  PRINT, 'reading data from ' + file
  OPENR, u, file, /GET_LUN
  fs = FSTAT(u)
  fs = fs.size
  s1 = ' '
  READF, u, s1                   ; comment line, used to extract type
  ft = STRMID(s1,0,1)
  IF fs GT 100 THEN READF, u, s1 ; template line, used to obtain line length
  CLOSE, u
  IF ft EQ '#' THEN BEGIN
    ; ASCII input file
    numneutrons = fs / STRLEN(s1) ; overestimate amount of neutrons
    extractData, numneutrons, data, file
  ENDIF ELSE BEGIN
    ; binary data
    ndoubles = 12L
    ; Unfortunately neutron data are padded with different
    ; amounts of bytes depending on cpu architecture and
    ; operating system. We have Intel x86 + Windows,
    ; Intel x 86 + Linux, Sparc + Solaris, and Alpha + Tru64
    IF STRMATCH(!Version.os, 'Win') THEN headsize = 16L ELSE $
      CASE !Version.arch OF
      'axp': headsize = 24L
      'alpha': headsize = 24L
      'sparc' : headsize = 16L
      ELSE: headsize = 12L
    ENDCASE
    head = BYTARR(headsize)
    chunksize = headsize + 8*ndoubles
    numneutrons = fs / chunksize
    IF numneutrons * chunksize NE fs THEN BEGIN
      numneutrons = 0
    ENDIF ELSE BEGIN
      dd = DBLARR(ndoubles)
      n = 0L
      data = DBLARR(ndoubles, numneutrons)
      OPENU,u,file
      WHILE NOT EOF(u) DO BEGIN
        READU, u, head,dd
        Data(*,n) = dd
        n = n+1
      ENDWHILE
    ENDELSE
  ENDELSE
  FREE_LUN, u
  IF numneutrons LE 0 THEN BEGIN
    myMESSAGE, 'cannot read file as vitess simulation output'
    RETURN
  ENDIF

  filenameFromPath, file
  toff = Data(0,*)
  wavelength = Data(1,*)
  probability = Data(2,*)
  x = Data(3,*)
  y = Data(4,*)
  z = Data(5,*)
  dx = Data(6,*)
  dy = Data(7,*)
  dz = Data(8,*)

  tempx = dx
  tempy = dy
  tempz = dz
  dy = ATAN(tempy/tempx)*360/(2*!PI)
  dz = ATAN(tempz/tempx)*360/(2*!PI)

  nc = 600
  nn = 100
  tmin = MIN(toff, MAX=tmax)
  time_hist = FLTARR(nc)
  time_value = FLTARR(nc)
  tbin = (tmax-tmin)/(nc-1)
  time_value = FINDGEN(nc)*tbin + tmin

  zmin = MIN(z, MAX=zmax)
  z_hist = FLTARR(nn)
  z_value = FLTARR(nn)
  zbin = (zmax-zmin)/(nn-1)
  z_value = FINDGEN(nn)*zbin + zmin

  ymin = MIN(y, MAX=ymax)
  y_hist  = FLTARR(nn)
  y_value = FLTARR(nn)
  ybin    = (ymax-ymin)/(nn-1)
  y_value = FINDGEN(nn)*ybin + ymin

  dy_hist = FLTARR(nn)
  dymin   = MIN(dy, MAX=dymax)
  dybin   = (dymax-dymin)/(nn-1)
  dy_value= FINDGEN(nn)*dybin + dymin

  dz_hist = FLTARR(nn)
  dzmin   = MIN(dz, MAX=dzmax)
  dzbin   = (dzmax-dzmin)/(nn-1)
  dz_value= FINDGEN(nn)*dzbin + dzmin

  yz_contour = FLTARR(nn,nn)

  ;velocity = 3995.4/wavelength
  ;velocity=81.81/(wavelength^2)	;velocity is actually energy in meV
  velocity=wavelength		;stick to wavelength
  vmin = MIN(velocity, MAX=vmax)
  velocity_hist  = FLTARR(nc)
  velocity_value = FLTARR(nc)
  vbin = ((vmax-vmin)/(nc-1))
  velocity_value = (FINDGEN(nc))*vbin+vmin

  FOR i=0l,numneutrons-1 DO BEGIN

    prob = probability(i)
    
    bb = LONG((dy(i)-dymin)/dybin)
    cc = LONG((dz(i)-dzmin)/dzbin)
    dy_hist(bb) = dy_hist(bb)+prob
    dz_hist(cc) = dz_hist(cc)+prob

    j  = LONG((toff(i)-tmin)/tbin)
    jj = LONG((velocity(i)-vmin)/vbin)
    l  = LONG((z(i)-zmin)/zbin)
    ll = LONG((y(i)-ymin)/ybin)
    time_hist(j) = time_hist(j)+prob
    velocity_hist(jj) = velocity_hist(jj)+prob
    z_hist(l)  = z_hist(l)+prob
    y_hist(ll) = y_hist(ll)+prob
    yz_contour(ll,l) = yz_contour(ll,l)+prob
  ENDFOR

  a = time_value
  b = time_hist
  c = velocity_value
  d = velocity_hist
  e = z_value
  f = z_hist
  g = y_value
  h = y_hist
  m = yz_contour
  p = dy_value
  q = dy_hist
  r = dz_value
  s = dz_hist
  RETURN
leave:
  myMessage, 'Error reading ' + file
END

PRO WriteData, fname, a,b
  OPENW, d, fname, /GET_LUN
  FOR i=0,N_ELEMENTS(a)-1 DO PRINTF, d, a(i),b(i)
  FREE_LUN, d
END

FUNCTION SCREENREAD, x0,y0,nx,ny,depth=depth
  ;check arguments
  IF N_ELEMENTS(x0) EQ 0 THEN x0 = 0
  IF N_ELEMENTS(y0) EQ 0 THEN y0 = 0
  IF N_ELEMENTS(nx) EQ 0 THEN nx = !d.x_vsize - x0
  IF N_ELEMENTS(ny) EQ 0 THEN ny = !d.y_vsize - y0
  IF N_ELEMENTS(depth) EQ 0 THEN depth = 1

  ;check for TVRD capable device
  tvrd_true = !d.flags AND 128
  IF (tvrd_true EQ 0) THEN myMESSAGE, $
    'TVRD is not supported on this device: ' + !d.name

  ;on devices which support windows, check for open window
  win_true = !d.flags AND 256
  IF (win_true GT 0) AND (!d.window LT 0) THEN myMESSAGE, $
    'No graphics windows are open'

  ;get IDL version number
  version = FLOAT(!version.release)

  ;get display depth
  depth = 8
  IF (win_true GT 0) THEN BEGIN
    IF (version GE 5.1) THEN BEGIN
      DEVICE, get_visual_depth = depth
    ENDIF ELSE BEGIN
      IF (!d.n_colors GT 256) THEN depth = 24
    ENDELSE
  ENDIF

  ;set decomposed color mode on 24-bit displays
  IF (depth GT 8) THEN BEGIN
    entry_decomposed = 0
    IF (version GT 5.1) THEN $
      DEVICE, get_decomposed=entry_decomposed
    DEVICE, decomposed=1
  ENDIF

  ;get the contents of the window
  IF (depth GT 8) THEN true = 1 ELSE true = 0
  image = TVRD(x0, y0, nx, ny, order=0, true=true)

  ;restore decomposed color mode on 24-bit displays
  IF depth GT 8 THEN DEVICE, decomposed=entry_decomposed

  RETURN, image
END


; event handling procedures

PRO DISPATCHEVENT, Event
  CASE Event OF
    'Browse file': browse_file
    'Read file': read_data
    'EXIT': EXIT
    ELSE: BEGIN
      COMMON SHARE1, a          ; if no data have been read, force reading data
      IF N_ELEMENTS(a) LE 0 THEN read_data, event ELSE performAction, event
    END
  ENDCASE
END

PRO performAction, e
  CASE e OF
    'Plot time': plot_time
    'Plot probability': plot_contour
    'Plot wavelength':  plot_velocity
    'Print': print_graph
    'Write JPEG': image_graph
    'Write time': outputfiletime
    'Write energy': outputfilevelocity
    'Horizonal flux': plot_misc, 1
    'Vertical flux': plot_misc, 2
    'Horizontal div': plot_misc, 3
    'Vertical div': plot_misc, 4
    'Plot energy': plot_energy
    'Plot all': plot_all
    ELSE:
  ENDCASE
END


PRO PLOT_TIME
  COMMON MCommon, file,user_operation
  COMMON SHARE1,a,b
  WINDOW, 0, xsize=650, ysize=350

  user_operation = 01
  PosPLOT, a,b, 'Time (ms)', 'Probability weighted intensity (arb. units)'
  WriteData, 'time_out.dat', a,b

;this part plots to a JPEG file
;if event.select eq 1 then begin
;	plot,c,d,xtitle='Velocity (m/s)',ytitle='Probability weighted intensity (arb. units)',$
;			title=file
;	image=screenread(depth=depth)
;	;write_jpeg,'test_velocity.jpg',image,true=1
;	write_jpeg,file+'.velocity.jpg',image,true=1
;endif

;this part prints - needs PRO file 'PRINTON'
;if event.select eq 1 then begin
;	printon, margin=1.0
;	plot,c,d,xtitle='Velocity (m/s)',ytitle='Probability weighted intensity (arb. units)'
;	printoff
;endif
END

PRO PLOT_VELOCITY
  WINDOW,0,xsize=650,ysize=350
  COMMON SHARE1, a,b,c,d
  COMMON MCommon, file,user_operation
  user_operation = 02
  PosPlot, c,d, 'Wavelength (Angstroms)', $
    'Probability weighted intensity (arb. units)', file, /Log
  WriteData, 'velocity_out.dat', c,d
END

PRO PLOT_ENERGY
  WINDOW,0,xsize=650,ysize=350
  COMMON SHARE1,a,b,c,d
  COMMON MCommon, file,user_operation

  user_operation = 02
  PosPlot, 81.81/(c^2), d, 'Energy (meV)', $
    'Probability weighted intensity (arb. units)', file, /Log
  WriteData, 'velocity_out.dat', c,d
END

PRO PLOT_MISC, i
  COMMON SHARE1,a,b,c,d,e,f,g,h,m,p,q,r,s
  WINDOW,0,xsize=650,ysize=200
  CASE i OF
    1: PosPLOT, g,h, 'horizontal position (cm)', 'horizontal integrated flux'
    2: PosPLOT, e,f, 'vertical position (cm)', 'vertical integrated flux'
    3: PosPLOT, p,q, 'horizontal divergence (degrees)', 'Prob. weighted intensity (arb.)'
    4: PosPLOT, r,s, 'vertical divergence (degrees)', 'Prob. weighted intensity (arb.)'
Else:
  ENDCASE
END


PRO PLOT_CONTOUR
  COMMON SHARE1,a,b,c,d,e,f,g,h,m
  COMMON MCommon, file,user_operation
  WINDOW,0,xsize=550,ysize=500
  user_operation = 03
  ContourPlot, m,g,e, 'y-position (cm)', 'z-position (cm)'

;window,1,xsize=100,ysize=100
;shade_surf,m,g,e
END



PRO PLOT_ALL
  COMMON SHARE1,a,b,c,d,e,f,g,h,m,p,q,r,s
  WINDOW,0,xsize=1000,ysize=800, retain=2
  
  !p.multi=[0,2,4,0,0]      ; 2 Colums ,4 rows
  
  PosPLOT, 81.81/(c^2), d, 'Energy (meV)', 'Intensity',$
;    'Energy', pos=[0.08, 0.78, 0.47, 0.98], /Log
  'Energy', /Log

  PosPLOT, c,d, 'Wavelength (Angstroms)', 'Intensity',$
    ;   'Wavelength', pos=[0.56,0.78,0.98,0.98], /Log, /noerase
   'Wavelength',  /Log

  PosPLOT, g,h, 'horizontal position (cm)', 'Horizontal integrated flux',$
    ;   'Horizontal Flux', pos=[0.08,0.5,0.47,0.7], /noerase
   'Horizontal Flux'

  PosPLOT, e,f, 'vertical position (cm)', 'Vertical integrated flux',$
    ;   'Vertical Flux', pos=[0.56,0.5,0.98,0.7], /noerase
  'Vertical Flux'

  PosPLOT, p,q, 'horizontal divergence (degrees)', 'Intensity',$
    ;  'Horizontal Divergence', pos=[0.08,0.23,0.47,0.42], /noerase
  'Horizontal Divergence'

  PosPLOT, r,s, 'vertical divergence (degrees)', 'Intensity',$
    ;  'Vertical Divergence', pos=[0.56,0.23,0.98,0.42], /noerase
   'Vertical Divergence'

  ContourPlot, m,g,e, 'y-position (cm)', 'z-position (cm)';,$
    ;   pos=[0.08,0.02,0.2,0.18], /noerase
  
END


PRO PRINTON, PAPER=PAPER, MARGIN=MARGIN, $
             PAGE_SIZE=PAGE_SIZE, INCHES=INCHES, ASPECT=ASPECT, $
             LANDSCAPE=LANDSCAPE, QUIET=QUIET

  COMMON MAINOPTS, slash, wdevice, pdevice, isidl
  
;- Check arguments

  IF KEYWORD_SET(landscape) THEN islandscape = 1 ELSE islandscape = 0
  IF N_ELEMENTS(paper) EQ 0 THEN paper = 'A4'
  IF N_ELEMENTS(margin) EQ 0 $
    THEN margin = 2.5 $
    ELSE IF KEYWORD_SET(inches) THEN margin = margin * 2.54

;- Check if Printer mode is active
  IF !d.name EQ pdevice THEN BEGIN
    myMESSAGE, 'PRINTER output is already active', /continue
    RETURN
  ENDIF

;- Check for IDL 5.2 or higher
  IF isidl AND (FLOAT(!version.release) LT 5.2) THEN $
    myMESSAGE, 'IDL 5.2 or higher is required'

;- Get ratio of character width/height to
;- screen width/height
  xratio = FLOAT(!d.x_ch_size) / FLOAT(!d.x_vsize)
  yratio = FLOAT(!d.y_ch_size) / FLOAT(!d.y_vsize)

;- Save current device information in common block
  COMMON PRINTON_INFORMATION, INF
  INF = {infostruct, DEVICE:!d.name, WINDOW:!d.window, font:!p.font, $
           xratio:xratio, yratio:yratio, background:!p.background, color:!p.color}

;- Get size of page (centimeters)
  widths  = [[ 8.5,  8.5, 11.0,  7.25] * 2.54, 21.0, 29.7]
  heights = [[11.0, 14.0, 17.0, 10.50] * 2.54, 29.7, 42.0]
  names   = ['LETTER', 'LEGAL', 'TABLOID', 'EXECUTIVE', $
              'A4', 'A3']
  index = WHERE(STRUPCASE(paper) EQ names, count)
  IF count NE 1 THEN BEGIN
    myMESSAGE, 'PAPER selection not supported', /continue
    RETURN
  ENDIF
  page_width  = widths(index(0))
  page_height = heights(index(0))

;- If page size was supplied, use it
  IF N_ELEMENTS(page_size) EQ 2 THEN BEGIN
    page_width  = page_size(0)
    page_height = page_size(1)
    IF KEYWORD_SET(inches) THEN BEGIN
      page_width  = page_width * 2.54
      page_height = page_height * 2.54
    ENDIF
  ENDIF

;- Compute aspect ratio of page when margins are subtracted
  page_aspect = FLOAT(page_height - 2.0 * margin) / $
    FLOAT(page_width  - 2.0 * margin)
  
;- Get aspect ratio of current graphics window
  IF !d.window GE 0 $
    THEN win_aspect = FLOAT(!d.y_vsize) / FLOAT(!d.x_vsize) $
    ELSE win_aspect = 512.0 / 640.0

;- If aspect ratio was supplied, use it
  IF N_ELEMENTS(aspect) EQ 1 THEN win_aspect = FLOAT(aspect)

;- Compute size of drawable area
;- (method used here is the same as PostScript method)
;- Compute offset of drawable area from page edges
;- (landscape method here is different
;-  than the PostScript method)
  IF islandscape THEN BEGIN
    IF win_aspect GE (1.0 / page_aspect) THEN BEGIN
      ysize = page_width - 2.0 * margin
      xsize = ysize / win_aspect
    ENDIF ELSE BEGIN
      xsize = page_height - 2.0 * margin
      ysize = xsize * win_aspect
    ENDELSE
    xoffset = (page_height - xsize) * 0.5
    yoffset = (page_width  - ysize) * 0.5
  ENDIF ELSE BEGIN
    IF win_aspect GE page_aspect THEN BEGIN
      ysize = page_height - 2.0 * margin
      xsize = ysize / win_aspect
    ENDIF ELSE BEGIN
      xsize = page_width - 2.0 * margin
      ysize = xsize * win_aspect
    ENDELSE
    xoffset = (page_width  - xsize) * 0.5
    yoffset = (page_height - ysize) * 0.5
  ENDELSE

;- Switch to Printer device
;- Note (1): Default units are centimeters
;- Note (2): Separate device commands are required!
  SET_PLOT, pdevice
  IF islandscape then DEVICE, /landscape
  DEVICE, scale_factor=1.0
  DEVICE, xsize=xsize, ysize=ysize, xoffset=xoffset, yoffset=yoffset
  IF isidl THEN BEGIN
    DEVICE, /index_color
    ;- Set character size
    xcharsize = round(inf.xratio * !d.x_vsize)
    ycharsize = round(inf.yratio * !d.y_vsize)
    DEVICE, set_character_size=[xcharsize, ycharsize]
  ENDIF ELSE BEGIN
    DEVICE, /Bold, filename='rvitess.ps'    
    !P.Background = 1
    !P.Color = 0
  ENDELSE
  
;- Report to user
  IF KEYWORD_SET(quiet) EQ 0 THEN $
    PRINT, 'Started PRINTER output'
END


PRO PRINTOFF, QUIET=QUIET
  COMMON MAINOPTS, slash, wdevice, pdevice, isidl
  COMMON Pinfo, pp
;- Check that Printer output is active
  IF !d.name NE pdevice THEN BEGIN
    myMESSAGE, 'PRINTER output not active: ' + $
      'nothing done', /continue
    RETURN
  ENDIF

;- Get entry device information from common block
  COMMON PRINTON_INFORMATION, INF
  IF N_ELEMENTS(INF) EQ 0 THEN BEGIN
    myMESSAGE, 'PRINTON was not called prior to PRINTOFF: ' + $
      'nothing done', /continue
    RETURN
  ENDIF

;- Close Printer device
  IF isidl THEN DEVICE, /Close_document ELSE DEVICE, /Close

;- Report to user
  IF KEYWORD_SET(quiet) EQ 0 THEN BEGIN
    IF N_ELEMENTS(pp) LE 0 THEN BEGIN
      pp = 1
      IF isidl THEN BEGIN
        help, /DEVICE           ; to show where the output went to
        PRINT, 'Do'
        PRINT, '  Result = DIALOG_PRINTERSETUP()'
        PRINT, 'to change printer settings!'
      ENDIF
    ENDIF
    IF isidl $
      THEN PRINT, 'Ended PRINTER output' $
      ELSE PRINT, 'Ended PRINT output to file rvitess.ps'
  ENDIF


;- Switch to entry graphics device
  SET_PLOT, inf.device

;- Restore window and font
  IF inf.window GE 0 THEN WSET, inf.window
  !P.font = inf.font
  !P.background = inf.background
  !P.color = inf.color
END


PRO PRINT_GRAPH
  COMMON SHARE1,a,b,c,d,e,f,g,h,m
  COMMON MCommon, file,user_operation
  printon, margin=1.0
  CASE user_operation OF
    1: PosPLOT, a,b, 'Time (ms)', 'Probability weighted intensity (arb. units)', file
    2: PosPLOT, c,d, 'Energy (meV)', 'Probability weighted intensity (arb. units)', file
    3: ContourPlot, m,g,e, 'Y-Position (cm)', 'Z-Position (cm)', file, nlevels=20
  ENDCASE
  printoff
END


PRO OUTPUTFILETIME
  COMMON SHARE1,a,b
  WriteData, 'time_out.dat', a,b
END

PRO OUTPUTFILEVELOCITY
  COMMON SHARE1,a,b,c,d
  WriteData, 'velocity_out.dat', 81.81/(c^2), d
  ; energy = 81.81/(c^2)
END

PRO IMAGE_GRAPH
  COMMON MCommon, file,user_operation
  image = screenread()
  CASE user_operation OF
    1: fn = 'time'
    2: fn = 'energy'
    3: fn = 'probability'
  ENDCASE
  write_jpeg, file+'.'+fn+'.jpg', image, true=1
END


PRO RVITESS, GROUP_LEADER=wGroup, _EXTRA=_VWBExtra_
  ; full declaration of common blocks, because this is performed first
  COMMON MCommon, file,user_operation
  COMMON SHARE1,a,b,c,d,e,f,g,h,m,p,q,r,s
  COMMON MAINOPTS, slash, wdevice, pdevice, isidl
  COMMON BNAMES, sn
  slash = '/'
  wdevice = 'X'
  user_operation = 01
  sn = ['Browse file', 'Read file', $
        'Plot time', 'Plot wavelength', 'Plot energy', 'Plot probability', $
        'Horizonal flux', 'Vertical flux', 'Horizontal div', 'Vertical div', $
        'Print', 'Write JPEG', 'Write time', 'Write energy', $
        'Plot all', 'EXIT']

  IF STRPOS(!prompt, 'WAVE') EQ 0 THEN isidl = 0 ELSE isidl = 1
  IF isidl $
    THEN rvit_idl, GROUP_LEADER=wGroup, _EXTRA=_VWBExtra_ $
    ELSE rvit_wave
END
