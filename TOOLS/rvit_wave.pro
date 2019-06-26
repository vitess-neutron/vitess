PRO ContourPlot, x,y,z, xt,yt, tit, pos=pos, nlevels=nlevels, noerase=noerase
  IF PARAM_PRESENT(nlevels) THEN a = 'nlevels' ELSE a = '90'
  IF N_ELEMENTS(tit) GT 0 THEN a = a + ', title=tit'
  IF PARAM_PRESENT(pos) THEN a = a + ', position=pos'
  IF PARAM_PRESENT(noerase) THEN a = a + ', /noerase'
  a = a +', charsize=1.5'
  rc = EXECUTE('CONTOUR, x,y,z, xtitle=xt, ytitle=yt, ' + $
    'xrange=[-4,4],yrange=[-4,4],xstyle=1,ystyle=1, nlevels=' + a)
END

PRO PosPLOT, x,y, xt,yt, tit, pos=pos, noerase=noerase, log=log
  IF N_ELEMENTS(tit) GT 0 THEN a = ', title=tit' ELSE a = ''
  IF PARAM_PRESENT(pos) THEN a = a + ', position=pos'
  IF PARAM_PRESENT(noerase) THEN a = a + ', /noerase'
  IF PARAM_PRESENT(log) THEN BEGIN
    a = a + ', ytype=1'
    sadd = '>1.0e-10'           ; do not plot negative or zero values with log mode
  ENDIF ELSE sadd = ''
  a = a +', charsize=1.5'
  rc = EXECUTE('PLOT, x,y' + sadd + ', xtitle=xt, ytitle=yt, psym=10' + a)
END


PRO FILENAMEFROMPATH, file
  f = file
  parsefilename, f, fileroot=file ; modifies variable file
END

PRO GETFILENAME, finalaction
  COMMON MCommon, file
  COMMON Cwid, textwid, top
  file = STRTRIM(WwGetValue(textwid),2)
  IF file EQ '' THEN Browse_file, finalaction
END

PRO MyMESSAGE, s
  COMMON Cwid, textwid, top
  rc = WwAlert(top, s)
END

PRO FileOK, wid, shell
  COMMON Cwid, textwid
  COMMON MCommon, file
  COMMON FinalActionCommon, faction
  file = STRTRIM(WwGetValue(wid), 2)
  rc = WwSetValue(textwid, file)
  status = WwSetValue(shell, /Close)
  IF faction EQ '' THEN RETURN
  read_data
  performAction, faction
END

PRO CancelCB, wid, shell
END

PRO Browse_File, finalaction
  COMMON Cwid, textwid
  COMMON FinalActionCommon, faction
  IF N_ELEMENTS(finalaction) GT 0 THEN faction = finalaction ELSE faction = ''
  w = WwFileSelection(textwid, 'FileOK', 'CancelCB', $
                      title='Select Vitess Output File', $
                      pattern='*.dat', position=[200,300])
END

PRO ButtonCB, wid, data
  dispatchEvent, WwGetvalue(wid)
END

PRO NEXTMB, b, j, n
  COMMON BNAMES, s
  rc = WwButtonBox(b, s(j:j+n-1), 'ButtonCB', /vertical, measure=4)
  j = j + n
END

PRO ButtonField
  COMMON Cwid, textwid, top
  ts = 'Vitess output reader'
  top = WwInit('Vitess', ts, layout, $
               title=ts, background='#dce2ff', position=[10,10], $
               font='-*-helvetica-*-r-normal-*-12-*', /form)
  w = layout
  n = 6
  b = LONARR(n)
  FOR i=0,n-1 DO BEGIN
    w = WwLayout(layout, /Form, Top=w)
    b(i) = w
  ENDFOR
  textwid = WwText(b(0), cols=48, label='Filename ')

  j = 0
  nextMB, b(1), j, 2
  nextMB, b(2), j, 4
  nextMB, b(3), j, 4
  nextMB, b(4), j, 4
  nextMB, b(5), j, 2

  status = WwSetValue(top, /Show)

  dispatchEvent, 'Plot time'
  wwloop
END

PRO EXTRACTDATA, n, data, fn
  data = DBLARR(12,n)
  IF !Version.os EQ 'Linux' THEN PRINT, !DPi ; seems to be necessary to initialize some interal library
  status = DC_READ_FREE(fn, data, nskip=1, /Col, resize=[1], get_columns = (4 + indgen(12)))
  s = SIZE(data)
  n = s(2)
  ; PRINT, data(*,0)
END

PRO RVIT_WAVE
  COMMON MAINOPTS, slash, wdevice, pdevice
  pdevice = 'PS'
  IF STRMATCH(!version.os, 'Win') THEN BEGIN
    slash = "\"                 ;"
    wdevice = 'WIN32'
  ENDIF ELSE BEGIN
    IF !Version.os EQ 'Linux' THEN PRINT, !DPi
    INFO, /DEVICE               ; to allow correct execution of device command
    tt = !d.display_depth
    DEVICE, true_color=tt
    !P.color = LONG('00'x)
    !P.background = LONG('ffffff'x)
  ENDELSE
  ButtonField
END
