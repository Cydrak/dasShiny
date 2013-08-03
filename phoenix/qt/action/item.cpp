namespace phoenix {

void pItem::setImage(const image& image) {
  qtAction->setIcon(CreateIcon(image));
}

void pItem::setText(string text) {
  qtAction->setText(QString::fromUtf8(text));
}

void pItem::constructor() {
  qtAction = new QAction(0);
  connect(qtAction, SIGNAL(triggered()), SLOT(onActivate()));
}

void pItem::destructor() {
  if(action.state.menu) action.state.menu->remove(item);
  delete qtAction;
  qtAction = nullptr;
}

void pItem::onActivate() {
  if(item.onActivate) item.onActivate();
}

}
