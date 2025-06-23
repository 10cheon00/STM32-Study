# 목적

STM32F103RB를 사용하는 Nucleo-F103RB 보드를 사용해 [2020-2 임베디드시스템설계](https://www.youtube.com/playlist?list=PLP4rlEcTzeFI68k2A57nevWkL2Ono0IL3) 동영상 강의를 따라 배우고, 활용하면서 만들어본 여러 프로젝트를 보관하는 리포지토리입니다.

# 개발환경

|환경|버전|
|---|---|
|OS|Windows 10 24H2(Desktop), MacOS Sequoia 15.5(24F74)(MacBook M2 Air) (노트북환경이 macos입니다.)|
|STM32CubeMX|6.14.1|
|STM32CubeProg|2.19.0|
|STM32CubeCLT|1.18.0|
|펌웨어|V24J46M32|
|Keil uVision5|5.42.00|
|Keil uVision5 ToolChain MDK-Lite|5.42.0.0|
|CMake|3.28.1|

# 실행방법

실행을 위한 코드와 ioc파일만 리포지토리에 있으므로, 먼저 CubeMX로 Generate Code를 하여 필요한 라이브러리 코드들을 생성한 후 Cmake를 사용해 VSCode에서 Task(Build + Flash)를 실행하면 됩니다.
