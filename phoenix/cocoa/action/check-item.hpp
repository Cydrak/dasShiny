@interface CocoaCheckItem : NSMenuItem {
@public
  phoenix::CheckItem* checkItem;
}
-(id) initWith:(phoenix::CheckItem&)checkItem;
-(void) activate;
@end

namespace phoenix {

struct pCheckItem : public pAction {
  CheckItem& checkItem;
  CocoaCheckItem* cocoaCheckItem = nullptr;

  bool checked();
  void setChecked(bool checked);
  void setText(string text);

  pCheckItem(CheckItem& checkItem) : pAction(checkItem), checkItem(checkItem) {}
  void constructor();
  void destructor();
};

}
