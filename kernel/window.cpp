#include "window.hpp"

#include "logger.hpp"
#include "font.hpp"

Window::Window(int width, int height, PixelFormat shadow_format) : width_{width}, height_{height} {
  data_.resize(height);
  for (int y = 0; y < height; ++y) {
    data_[y].resize(width);
  }

  FrameBufferConfig config{};
  config.frame_buffer = nullptr;
  config.horizontal_resolution = width;
  config.vertical_resolution = height;
  config.pixel_format = shadow_format;

  if (auto err = shadow_buffer_.Initialize(config)) {
    Log(kError, "failed to initialize shadow buffer: %s at %s:%d\n",
        err.Name(), err.File(), err.Line());
  }
}

void Window::DrawTo(FrameBuffer& dst, Vector2D<int> pos, const Rectangle<int>& area) {
  if (!transparent_color_) {
    Rectangle<int> window_area{pos, Size()};
    Rectangle<int> intersection = area & window_area;
    dst.Copy(intersection.pos, shadow_buffer_, {intersection.pos - pos, intersection.size});
    return;
  }

  const auto tc = transparent_color_.value();
  auto& writer = dst.Writer();
  for (int y = std::max(0, 0 - pos.y);
       y < std::min(Height(), writer.Height() - pos.y);
       ++y) {
    for (int x = std::max(0, 0 - pos.x);
         x < std::min(Width(), writer.Width() - pos.x);
         ++x) {
      const auto c = At(Vector2D<int>{x, y});
      if (c != tc) {
        writer.Write(pos + Vector2D<int>{x, y}, c);
      }
    }
  }
}

void Window::SetTransparentColor(std::optional<PixelColor> c) {
  transparent_color_ = c;
}

Window::WindowWriter* Window::Writer() {
  return &writer_;
}

const PixelColor& Window::At(Vector2D<int> pos) const{
  return data_[pos.y][pos.x];
}

void Window::Write(Vector2D<int> pos, PixelColor c) {
  data_[pos.y][pos.x] = c;
  shadow_buffer_.Writer().Write(pos, c);
}

int Window::Width() const {
  return width_;
}

int Window::Height() const {
  return height_;
}

Vector2D<int> Window::Size() const {
  return {width_, height_};
}

void Window::Move(Vector2D<int> dst_pos, const Rectangle<int>& src) {
  shadow_buffer_.Move(dst_pos, src);
}

void Window::SetTitle(char* title) {
  title_ = title;
}

char* Window::GetTitle() {
  return title_;
}


namespace {
  const int kCloseButtonWidth = 16;
  const int kCloseButtonHeight = 13;
  const char close_button[kCloseButtonHeight][kCloseButtonWidth + 1] = {
    "::::::::::::::::",
    "::::::::::::::::",
    "::::::::::::::::",
    "::::@@::::@@::::",
    ":::::@@::@@:::::",
    "::::::@@@@::::::",
    ":::::::@@:::::::",
    "::::::@@@@::::::",
    ":::::@@::@@:::::",
    "::::@@::::@@::::",
    "::::::::::::::::",
    "::::::::::::::::",
    "::::::::::::::::",
  };

  constexpr PixelColor ToColor(uint32_t c) {
    return {
      static_cast<uint8_t>((c >> 16) & 0xff),
      static_cast<uint8_t>((c >> 8) & 0xff),
      static_cast<uint8_t>(c & 0xff)
    };
  }
}

void DrawWindow(PixelWriter& writer, const char* title) {
  auto fill_rect = [&writer](Vector2D<int> pos, Vector2D<int> size, uint32_t c) {
    FillRectangle(writer, pos, size, ToColor(c));
  };
  const auto win_w = writer.Width();
  const auto win_h = writer.Height();

  // color
  uint32_t backgroundColor = 0xc6c6c6;
  uint32_t borderColor = 0x000000;
  uint32_t titleBackgroundColor = 0x848484;
  uint32_t titleFontColor = 0xffffff;
  uint32_t closeButtonBackgroundColor = 0xffffff;
  uint32_t closeButtonCloseIconColor = borderColor;
  if (is_darkmode) {
    backgroundColor = 0x0a0e12;
    borderColor = 0x1682a4;
    titleBackgroundColor = 0x082734;
    titleFontColor = 0x1788ac;
    closeButtonBackgroundColor = 0x104c65;
    closeButtonCloseIconColor = borderColor;
  }

  fill_rect({0, 0},         {win_w, 1},             borderColor); // ボーダー上
  fill_rect({0, 0},         {1, win_h},             borderColor); // ボーダー左
  fill_rect({win_w - 1, 0}, {1, win_h},             borderColor); // ボーダー右
  fill_rect({1, 1},         {win_w - 2, win_h - 2}, backgroundColor); // 背景
  fill_rect({1, 1},         {win_w - 2, 22},        titleBackgroundColor); // タイトル
  fill_rect({0, win_h - 1}, {win_w, 1},             borderColor); // ボーダー下

  // タイトル文字
  WriteString(writer, {10, 4}, title, ToColor(titleFontColor));

  // 閉じるボタン
  for (int y = 0; y < kCloseButtonHeight; ++y) {
    for (int x = 0; x < kCloseButtonWidth; ++x) {
      PixelColor c = ToColor(0xffffff);
      if (close_button[y][x] == '@') {
        c = ToColor(closeButtonCloseIconColor);
      } else if (close_button[y][x] == '$') {
        c = ToColor(0x848484);
      } else if (close_button[y][x] == ':') {
        c = ToColor(closeButtonBackgroundColor);
      }
      writer.Write({win_w - 5 - kCloseButtonWidth + x, 5 + y}, c);
    }
  }
}
