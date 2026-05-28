# chd コマンド

ディレクトリ移動コマンド

## 概要(Overview) 

Windows のコンソール画面でディレクトリの移動をカーソルの選択操作で行うコマンド。
本プログラムは、Google Gemini に全て作らせました。

## デモ画面(Demo)

<img width="717" height="358" alt="Image" src="https://github.com/user-attachments/assets/9ab64859-ec87-418e-bef1-7b3ea352f5ef" />

## 導入方法(Setup)

ソースファイルとバッチファイルは、UTF-8 では無く、Shift-JIS(ANSI) で保存します。 
Visual Studio でコンパイルを行う。 
cl chds.c 

chd.bat、chds.exe をパスの通ったディレクトリに置く。 

## 操作方法(Usage)

コンソール画面で、chd と入力 
移動したいディレクトリを選択すると、中が表示される。 
「..」を選択すると親ディレクトリに戻る。 
「.」を選択すると終了し、そのディレクトリに移動する。 
[ESC]キーを押すと移動せず終了する。 



