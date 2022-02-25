@echo off
SETLOCAL

IF ""=="%Config%" SET Config=Debug
IF ""=="%ExeDir%" SET ExeDir=..\%Config%
IF ""=="%Params%" SET Params=%*

FOR %%K in (220 440) do (
  FOR %%F in (sin tri squ) do (
    FOR %%B in (8 16 24 32) do (
      %ExeDir%\makeWAV sec=1 ch=1 key=%%K %Params% %%F %%B %%F%%B_%%K.wav
    )
  )
  %ExeDir%\PcmDataTest key=%%K %Params% > %%K.csv
  dir *%%K*.*
)
