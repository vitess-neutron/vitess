PRO ContourPlot, x,y,z, xt,yt, tit, pos=pos, nlevels=nlevels, noerase=noerase
  IF KEYWORD_SET(nlevels) THEN a = 'nlevels' ELSE a = '90'
  IF N_ELEMENTS(tit) GT 0 THEN a = a + ', title=tit'
  IF KEYWORD_SET(pos) THEN a = a + ', position=pos'
  IF KEYWORD_SET(noerase) THEN a = a + ', /noerase'
  a = a +', charsize=1.5'
  rc = EXECUTE('CONTOUR, x,y,z, /fill, xtitle=xt, ytitle=yt, ' + $
    'xrange=[-4,4],yrange=[-4,4],xstyle=1,ystyle=1, nlevels=' + a)
END

PRO PosPLOT, x,y, xt,yt, tit, pos=pos, noerase=noerase, log=log
  IF N_ELEMENTS(tit) GT 0 THEN a = ', title=tit' ELSE a = ''
  IF KEYWORD_SET(log) THEN a = a + ', /ylog'
  IF KEYWORD_SET(pos) THEN a = a + ', position=pos'
  IF KEYWORD_SET(noerase) THEN a = a + ', /noerase'
  a = a +', charsize=1.5'
  rc = EXECUTE('PLOT, x,y, xtitle=xt, ytitle=yt, psym=10' + a)
END

PRO FILENAMEFROMPATH, file
  COMMON MAINOPTS, slash

  ;isolate filename from full path
  dotpos = STRPOS(file, '.', /REVERSE_SEARCH)
  slashpos = STRPOS(file, slash, /REVERSE_SEARCH)
  OldFileLocation = STRMID(file, 0, slashpos+1)
  OldFileName = STRMID(file, slashpos+1, dotpos-slashpos-1)
  ;modify file
  file = OldFileName
END

PRO MYMESSAGE,s
  print, s
END

PRO GETFILENAME, finalaction
  COMMON Mcommon, file, user_operation
  COMMON Cwid, textwid
  WIDGET_CONTROL, textwid, GET_VALUE=f
  file = STRING(f(0))
  IF file NE '' THEN RETURN

  Browse_file, finalaction
  WIDGET_CONTROL, textwid, GET_VALUE=f
  file = STRING(f(0))
END

PRO TLB_EVENT, Event
  IF (TAG_NAMES(Event,/structure_name) NE 'WIDGET_BUTTON') THEN RETURN
  WIDGET_CONTROL, event.id, GET_VALUE=val
  dispatchEvent, val
END

PRO MButton, b, va
  rc = Widget_Button(b, VALUE=va, /ALIGN_CENTER, xsize=120)
END

PRO NEXTMB, b, j, n
  COMMON bnames, s
  FOR k=0,n-1 DO BEGIN
    MButton, b, s(j)
    j = j + 1
  ENDFOR
END

PRO Browse_File, finalaction
  COMMON Mcommon, file,user_operation
  COMMON Cwid, textwid
  file = Dialog_Pickfile(TITLE='Choose the Vitess file to read',$
                         Filter = '*.dat', /Must_exist)
  WIDGET_CONTROL, textwid, SET_VALUE=file
  read_data
  IF N_ELEMENTS(finalaction) LE 0 THEN RETURN
  IF finalaction NE '' THEN performAction, finalaction
END


PRO TLB, GROUP_LEADER=wGroup, _EXTRA=_VWBExtra_
  COMMON Cwid, textwid

  tlb = widget_base(/Column, tlb_frame_attr=1, $
                    title='Vitess output reader', uname='tlb')
  n = 6
  b = LONARR(n)
  FOR i=0,n-1 DO b(i) = widget_base(tlb, row=1, base_align_center=1)

  textwid = Widget_text(b(0), xsize=72, /Editable)
  j = 0
  nextMB, b(1), j, 2
  nextMB, b(2), j, 4
  nextMB, b(3), j, 4
  nextMB, b(4), j, 4
  nextMB, b(5), j, 2

  Widget_Control, /REALIZE, tlb
  dispatchEvent, 'Plot time'
  XManager, 'tlb', tlb, /NO_BLOCK
END

FUNCTION ExtractDoubles, s
  RETURN, DOUBLE(STRSPLIT(STRTRIM(STRCOMPRESS(s),2),' ',/Extract))
END

PRO EXTRACTDATA, numneutrons, data, fn
  OPENR, u, fn, /GET_LUN
  nmax = 32235L
  data = DBLARR(12, numneutrons)
  s = ' '
  READF, u, s                   ; skip
  n = 0L
  WHILE NOT EOF(u) DO BEGIN
    READF, u, s
    data(*,n) = extractDoubles(STRMID(s,18,256))
    n = n+1
  ENDWHILE
  data = data(*,0:n-1)
  numneutrons = n
  FREE_LUN, u
END

PRO RVIT_IDL, GROUP_LEADER=wGroup, _EXTRA=_VWBExtra_
  COMMON MAINOPTS, slash, wdevice, pdevice
  pdevice = 'PRINTER'
  IF !Version.os_family EQ 'Windows' THEN BEGIN
    slash = "\"                 ;"
    wdevice = 'WIN'
  ENDIF
  tlb, GROUP_LEADER=wGroup, _EXTRA=_VWBExtra_
END
