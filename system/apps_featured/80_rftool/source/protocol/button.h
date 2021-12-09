class CButton : public CProtocol
{
public:
  virtual int Frequency() override
  {
    return 433920000UL;
  }

  virtual void Example(CAttributes& attributes) override
  {
    attributes["length"] = 64;
    attributes["start"] = 2;
    attributes["shortDuration"] = 333;
    attributes["data_0"] = 0x12345600;
    attributes["data_1"] = 0;
  }

  virtual bool Demodulate(const CArray<uint16_t>& pulse, CAttributes& attributes) override
  {
    uint8_t nibblesData[16];
    CArray<uint8_t> b(nibblesData, COUNT(nibblesData));

    int length = 0;
    int start = 0;
    // The duration in microseconds of a short pulse.
    int shortDuration = 0;
    if (!PulseToBytes(pulse, b, length, start, shortDuration))
      return false;

    BitstreamToAttributes(b, length, attributes);
    attributes["start"] = start;
    attributes["shortDuration"] = shortDuration;
    if (pulse.GetSize() >= 6) {
      attributes["p0"] = pulse[0];
      attributes["p1"] = pulse[1];
      attributes["p2"] = pulse[2];
      attributes["p3"] = pulse[3];
      attributes["p4"] = pulse[4];
      attributes["p5"] = pulse[5];
    }
    return true;
  }

  virtual bool Modulate(const CAttributes& attributes, CArray<uint16_t>& pulse) override
  {
    uint8_t nibblesData[16];
    CArray<uint8_t> b(nibblesData, COUNT(nibblesData));

    int length = 0;
    AttributesToBitstream(attributes, b, length);
    int shortDuration = attributes["shortDuration"];
    return BytesToPulse(b, length, shortDuration, pulse);
  }

  virtual int PulseDivisor() override { return 333; }

private:

  int PulseLen(int microseconds, int shortDuration)
  {
    // Convert pulse duration in microseconds to a multiple of the shortest pulse length, rounding
    // to the closest match.
    return (microseconds + shortDuration / 2) / shortDuration;
  }

  bool PulseToBytes(const CArray<uint16_t>& pulse, CArray<uint8_t>& bytes, int& length, int& start, int& shortDuration)
  {
    int i;
    // Look for a long pulse (longer than 3 ms) to start.
    for (i = 0; i < pulse.GetSize() - 4; i++)
    {
      if (pulse[i] >= 3000)
      {
        i++;
        break;
      }
    }

    start = i;

    if (pulse.GetSize() - i < 10) {
      // If there are not many pulses left, it can't be this protocol.
      return false;
    }

    // Use the first two pulses to figure out the scale.
    shortDuration = (pulse[i] + pulse[i + 1]) / 4;

    // Do the actual decoding.
    length = 0;
    uint8_t bits = 0;
    for (; i < pulse.GetSize() - 1 && bytes.GetSize() < 16; i += 2)
    {
      int p0 = PulseLen(pulse[i], shortDuration);
      int p1 = PulseLen(pulse[i + 1], shortDuration);
      if (p0 == 3 && p1 == 1)
      {
        // 1 bit
        bits = bits << 1 | 1;
      }
      else if (p0 == 1 && p1 == 3)
      {
        // 0 bit
        bits = bits << 1;
      }
      else if (p0 >= 10 || p1 >= 10)
      {
        // We've reached the end of the code before it repeats.
        break;
      }
      else
      {
        return false;
      }

      ++length;

      if ((length & 7) == 0)
      {
        bytes.Add(bits);
        bits = 0;
      }
    }

    // Include a last incomplete byte, if any.
    if (bits != 0)
    {
      bytes.Add(bits);
    }

    return true;
  }

  bool BytesToPulse(const CArray<uint8_t>& bytes, int length, int shortDuration, CArray<uint16_t>& pulse)
  {
    pulse.Add(shortDuration * 1);
    pulse.Add(shortDuration * 30);

    for (int i = 0; i < length; i++)
    {
      uint8_t mask = 1 << (7 - (i & 7));
      bool bit = (bytes[i / 8] & mask) != 0;
      if (bit)
      {
        pulse.Add(shortDuration * 3);
        pulse.Add(shortDuration * 1);
      } else
      {
        pulse.Add(shortDuration * 1);
        pulse.Add(shortDuration * 3);
      }
    }
    return true;
  }

  virtual void GetName(char* name) override
  {
    strcpy(name, "Button");
  }

  virtual void GetDescription(CAttributes& attributes, char* desc) override
  {
    int length = attributes["length"];
    uint32_t d0 = attributes["data_0"];
    uint32_t d1 = attributes["data_1"];
    int start = attributes["start"];
    int shortDuration = attributes["shortDuration"];

    if (length == 24) {
      sprintf(desc, "%d bits (%d,%duS): <%06x>", length, start, shortDuration, (int)d0 >> 8);
    } else {
      sprintf(desc, "%d bits (%d,%duS): <%08x %08x>", length, start, shortDuration, (int)d0, (int)d1);
    }
  }

  virtual const char* GetString(int i) override { return nullptr; }
};
