cmake_minimum_required(VERSION 2.8)

# ソリューションファイルの名前を指定する
project( photo2value )

link_directories($ENV{TESSERACT_LIB_DIR})
add_executable(photo2value src/Main.cpp)

include_directories($ENV{TESSERACT_INCLUDE_DIR})
# OpenCVのパッケージを探す
find_package(OpenCV REQUIRED)

# OpenCVが見つかった場合のみ設定を行う
if(OpenCV_FOUND)
  # インクルードパスを指定
  include_directories(${OpenCV_INCLUDE_DIRS})

  # リンクするライブラリ指定
  target_link_libraries(photo2value ${OpenCV_LIBS})
endif(OpenCV_FOUND)

target_link_libraries(photo2value tesseract)

