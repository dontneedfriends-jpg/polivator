#ifndef DISPLAY_H
#define DISPLAY_H

class Display {
public:
  Display();
  ~Display();
  void begin();
  void update();
  void showMessage(const char* message);

private:
  // Display-specific members (e.g., for e-ink)
};

#endif // DISPLAY_H
