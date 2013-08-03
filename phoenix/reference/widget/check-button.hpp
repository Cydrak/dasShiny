namespace phoenix {

struct pCheckButton : public pWidget {
  CheckButton& checkButton;

  bool checked();
  void setChecked(bool checked);
  void setText(string text);

  pCheckButton(CheckButton& checkButton) : pWidget(checkButton), checkButton(checkButton) {}
  void constructor();
  void destructor();
};

}
