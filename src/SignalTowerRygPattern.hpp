#pragma once

#include <bits_asukiaaa.hpp>

namespace SignalTowerRygPattern {

static String normalizeColorStr(String state) {
  String result = "";
  static const String targets = "ryg";
  for (uint8_t i = 0; i < targets.length(); ++i) {
    char target = targets[i];
    if (state.indexOf(target) >= 0) {
      result += target;
    }
  }
  if (result.length() == 0) {
    result = 'x';
  }
  return result;
}

class ColorState {
 public:
  bool g, y, r;
  ColorState(String initialState = "") { update(initialState); }
  void update(String state) {
    String str = normalizeColorStr(state);
    g = str.indexOf('g') >= 0;
    y = str.indexOf('y') >= 0;
    r = str.indexOf('r') >= 0;
  }

  String buildStr() const {
    String str;
    if (r) {
      str += 'r';
    }
    if (y) {
      str += 'y';
    }
    if (g) {
      str += 'g';
    }
    if (str == "") {
      str = 'x';
    }
    return str;
  }

  void reset() { r = y = g = false; }

  bool operator==(const ColorState& target) const {
    return g == target.g && y == target.y && r == target.r;
  }
  bool operator!=(const ColorState& target) const {
    return g != target.g || y != target.y || r != target.r;
  }
};

class Item {
 public:
  Item(String str = "") { updateFromStr(str); }

  void updateFromStr(String str) {
    reset();
    auto indexMinus = str.indexOf("-");
    auto inputStrColor = indexMinus >= 0 ? str.substring(0, indexMinus) : str;
    color.update(inputStrColor);
    if (indexMinus >= 0 && str.length() > (uint32_t)(indexMinus + 1)) {
      auto strMs = str.substring(indexMinus + 1);
      msFor = strMs.toInt();
    }
  }

  static const size_t lenBytes = 4;
  void updateFromBytes(const uint8_t* bytes) {
    parseByteColors(bytes[0]);
    // bytes[1];  // reserved for future use
    msFor = bits_asukiaaa::readUint16FromBytes(&bytes[2]);
  }
  void toBytes(uint8_t* bytes) const {
    bytes[0] = buildByteColors();
    bytes[1] = 0;  // reserved
    bits_asukiaaa::assignUint16ToBytes(&bytes[2], msFor);
  }
  String toStr() const { return getStrColor() + "-" + String(msFor); }

  bool getRed() const { return color.r; }
  bool getYellow() const { return color.y; }
  bool getGreen() const { return color.g; }
  const ColorState* getColorP() const { return &color; }
  uint16_t getMsFor() const { return msFor; }
  String getStrColor() const { return color.buildStr(); }

  bool isActive() const { return msFor > 0; }

  void reset() {
    color.reset();
    msFor = 0;
  }

  bool operator==(const Item& target) const {
    return color == target.color && msFor == target.msFor;
  }
  bool operator!=(const Item& target) const {
    return color != target.color || msFor != target.msFor;
  }

 private:
  uint16_t msFor;
  ColorState color;

  uint8_t buildByteColors() const {
    using bits_asukiaaa::setBitTrue;
    uint8_t v = 0;
    if (color.r) {
      setBitTrue(&v, 0);
    }
    if (color.y) {
      setBitTrue(&v, 1);
    }
    if (color.g) {
      setBitTrue(&v, 2);
    }
    return v;
  }

  void parseByteColors(uint8_t v) {
    using bits_asukiaaa::isBitTrue;
    color.r = isBitTrue(v, 0);
    color.y = isBitTrue(v, 1);
    color.g = isBitTrue(v, 2);
  }
};

class ArraySet {
 public:
  static const size_t lenItems = 12;

  ArraySet(Item* items, size_t lenItems) {
    for (size_t i = 0; i < lenItems; ++i) {
      if (i + 1 >= this->lenItems) {
        break;
      }
      this->items[i] = items[i];
    }
  }

  ArraySet(String str = "") { updateFromStr(str); }

  void updateFromBytes(const uint8_t* bytes, size_t lenBytes) {
    for (size_t i = 0; i < lenItems; ++i) {
      auto regStart = i * Item::lenBytes;
      if (regStart + Item::lenBytes >= lenBytes) {
        return;
      }
      items[i].updateFromBytes(&bytes[regStart]);
    }
  }
  void toBytes(uint8_t* bytes, size_t lenBytes) const {
    for (size_t i = 0; i < lenItems; ++i) {
      auto regStart = i * Item::lenBytes;
      if (regStart + Item::lenBytes >= lenBytes) {
        return;
      }
      items[i].toBytes(&bytes[regStart]);
    }
  }

  String toStr() const {
    String str = "";
    for (size_t i = 0; i < lenItems; ++i) {
      if (i != 0) {
        str += ",";
      }
      str += items[i].toStr();
    }
    return str;
  }

  String toStrWithoutInActiveInfo() const {
    if (!hasActiveItem()) {
      return items[0].getStrColor();
    }
    String str = "";
    for (size_t i = 0; i < lenItems; ++i) {
      auto item = items[i];
      if (!item.isActive()) {
        continue;
      }
      if (str != "") {
        str += ",";
      }
      str += item.toStr();
    }
    return str;
  }

  void updateFromStr(String str) {
    size_t indexStart = 0;
    size_t indexEnd = 0;
    for (size_t i = 0; i < lenItems; ++i) {
      auto item = &items[i];
      if (indexEnd >= str.length()) {
        item->reset();
        continue;
      }
      if (indexEnd != 0) {
        indexStart = indexEnd + 1;
      }
      indexEnd = str.indexOf(',', indexStart);
      if (indexEnd < 0) {
        indexEnd = str.length();
      }
      auto strItem = str.substring(indexStart, indexEnd);
      item->updateFromStr(strItem);
    }
  }

  const Item* getItemP(size_t index) const {
    if (index < lenItems) {
      return &items[index];
    } else {
      return &itemBlank;
    }
  }

  void updatePattern(const ArraySet& arraySet) {
    for (size_t i = 0; i < lenItems; ++i) {
      items[i] = *arraySet.getItemP(i);
    }
  }

  bool hasActiveItem() const {
    for (size_t i = 0; i < lenItems; ++i) {
      if (items[i].isActive()) {
        return true;
      }
    }
    return false;
  }

  bool operator==(const ArraySet& target) const {
    for (size_t i = 0; i < lenItems; ++i) {
      if (items[i] != (*target.getItemP(i))) {
        return false;
      }
    }
    return true;
  }
  bool operator!=(const ArraySet& target) const { return !(*this == target); }

 private:
  Item items[lenItems];
  const Item itemBlank;

  void updateItem(size_t index, const Item& item) {
    if (index < lenItems) {
      items[index] = item;
    }
  }

  void updateItemFromStr(size_t index, String str) {
    if (index < lenItems) {
      items[index].updateFromStr(str);
    }
  }

  void updateItemFromBytes(size_t index, const uint8_t* bytes) {
    if (index < lenItems) {
      items[index].updateFromBytes(bytes);
    }
  }
};

class Manager : public ArraySet {
 public:
  Manager() {}
  Manager(const ArraySet& arraySet) { updatePattern(arraySet); }
  void updateIfNeeded() {
    if (!hasActiveItem()) {
      indexCurrentItem = 0;
      return;
    }
    if (millis() - itemFrom > getCurrentItem().getMsFor()) {
      auto indexNext = getNextIndex(indexCurrentItem);
      while (true) {
        if (indexNext == indexCurrentItem || getItemP(indexNext)->isActive()) {
          indexCurrentItem = indexNext;
          break;
        }
        indexNext = getNextIndex(indexNext);
      }
      itemFrom = millis();
    }
  }

  void updatePattern(const ArraySet& arraySet) {
    if (*this != arraySet) {
      resetInternalClock();
      ArraySet::updatePattern(arraySet);
    }
  }

  void updateFromStr(String str) {
    if (toStr() != str) {
      resetInternalClock();
      ArraySet::updateFromStr(str);
    }
  }

  void resetInternalClock() { itemFrom = millis(); }
  ColorState getCurrentColorState() const {
    return *getCurrentItem().getColorP();
  }
  String getCurrentStr() const { return getCurrentItem().getStrColor(); }
  Item getCurrentItem() const { return *getItemP(indexCurrentItem); }
  size_t getIndexCurrentItem() const { return indexCurrentItem; }

 private:
  size_t getNextIndex(size_t i) {
    ++i;
    if (i + 1 > lenItems) {
      return 0;
    } else {
      return i;
    }
  }
  size_t indexCurrentItem = 0;
  unsigned long itemFrom = 0;
};

namespace Preset {
const ArraySet Demo("r-3000,y-3000,g-3000,x-3000,ryg-3000,x-3000");
const ArraySet RedBlinkSlow("r-1000,x-1000");
const ArraySet Red("r-1000");
const ArraySet YellowBlinkSlow("y-1000,x-1000");
const ArraySet Yellow("y-1000");
const ArraySet GreenBlinkSlow("g-1000,x-1000");
const ArraySet Green("g-1000");
const ArraySet Blank;

const ArraySet Default = YellowBlinkSlow;
const ArraySet EmergencyStop = RedBlinkSlow;
const ArraySet FootIgnoringRos = Yellow;
const ArraySet DetectingBumper = Red;
}  // namespace Preset

}  // namespace SignalTowerPattern
