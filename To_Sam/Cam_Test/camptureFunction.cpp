void getStream() {
  camera_fb_t *fb = NULL; // img place
  size_t _jpg_buf_len = 0; // img length
  uint8_t *_jpg_buf = NULL; // img local
  char part_buf[64]; // a part of img (use to send to web)

  // Send HTTP header
  server.sendContent_P("HTTP/1.1 200 OK\r\nContent-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n");

  while (true) {
    fb = esp_camera_fb_get(); // get img

    // preprocessing
    if (!fb) {
      Serial.println("Camera capture failed"); // img get failed
      break;
    } else {
      if (fb->format != PIXFORMAT_JPEG) { // type change
        bool converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
        esp_camera_fb_return(fb);
        fb = NULL;

        if (!converted) {
          Serial.println("JPEG compression failed");
          break;
        }
      } else {
        _jpg_buf_len = fb->len;
        _jpg_buf = fb->buf;
      }
    }

    // send img
    size_t hlen = snprintf(part_buf, 64, "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", _jpg_buf_len);
    server.sendContent_P(part_buf, hlen); // 发送HTTP头信息
    server.sendContent_P((const char *)_jpg_buf, _jpg_buf_len); // 发送JPEG图像数据
    server.sendContent_P("\r\n--frame\r\n", 14); // 发送帧边界

    // release resource and error
    if (fb) {
      esp_camera_fb_return(fb); // 释放帧缓冲区
      fb = NULL;
      _jpg_buf = NULL;
    } else if (_jpg_buf) {
      free(_jpg_buf); // 释放JPEG缓冲区
      _jpg_buf = NULL;
    }
  }
}

void getPhoto() {
  camera_fb_t * fb = esp_camera_fb_get();
  if (fb) {
    server.sendHeader("Content-Type", "image/jpeg");
    server.sendHeader("Content-Disposition", "inline", true);
    server.sendHeader("Cache-Control", "no-cache");
    server.sendContent(fb->buf, fb->len);
    esp_camera_fb_return(fb);
  }
}