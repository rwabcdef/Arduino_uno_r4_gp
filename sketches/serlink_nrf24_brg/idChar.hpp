
#ifndef IDCHAR_HPP_
#define IDCHAR_HPP_

namespace HardMod::Std
{
class FixedIdChar
{
  protected:
    char id = '-';

  public:
    FixedIdChar(char value): id(value) {}
    char getId() { return this->id; }
};

class VariableIdChar
{
  protected:
    char id = '-';

  public:
    void setId(char value) { this->id = value; }
    char getId() { return this->id; }
};

} // end namespace HardMod::Std

#endif