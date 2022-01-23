@echo off
SETLOCAL

IF ""=="%Config%" SET Config=Debug
IF ""=="%ExeDir%" SET ExeDir=..\%Config%
IF ""=="%Params%" SET Params=%1

FOR %%K in (220 440) do (
  FOR %%F in (sin tri squ) do (
    FOR %%B in (8 16 32) do (
      %ExeDir%\makeWAV sec=1 ch=1 key=%%K %Params% %%F %%B %%F%%B_%%K_1ch.wav
    )
  )
  %ExeDir%\PcmDataTest ch=1 key=%%K %Params% > %%K_1ch.csv
  dir *%%K*_1ch.*
)
