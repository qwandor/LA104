class CButton : public CProtocol
{
public:
  virtual int Frequency() override
  {
    return 433850000UL;
  }

  virtual void Example(CAttributes& attributes) override
  {
    attributes["length"] = 64;
    attributes["data_0"] = 0x12345678;
    attributes["data_1"] = 0xabcdef01;
  }

  virtual bool Demodulate(const CArray<uint16_t>& pulse, CAttributes& attributes) override
  {
    uint8_t nibblesData[16];
    CArray<uint8_t> b(nibblesData, COUNT(nibblesData));

    int length = 0;
    int start = 0;
    if (!PulseToBytes(pulse, b, length, start))
      return false;

    BitstreamToAttributes(b, length, attributes);
    attributes["start"] = start;
    return true;
  }

  virtual bool Modulate(const CAttributes& attr, CArray<uint16_t>& pulse) override
  {
    uint8_t nibblesData[16];
    CArray<uint8_t> b(nibblesData, COUNT(nibblesData));

    int length = 0;
    AttributesToBitstream(attr, b, length);
    return BytesToPulse(b, length, pulse);
  }

  virtual int PulseDivisor() override { return 333; }

private:

  int PulseLen(int microseconds)
  {
    // Convert pulse duration in microseconds to a multiple of the shortest pulse length, rounding
    // to the closest match.
    return (microseconds + 333 / 2) / 333;
  }

  int PulseDuration(int ticks)
  {
    return ticks * 333;
  }

  bool PulseToBytes(const CArray<uint16_t>& pulse, CArray<uint8_t>& bytes, int& length, int& start)
  {
    int i;
    // Look for a long pulse to start.
    for (i = 0; i < pulse.GetSize() - 4; i++)
    {
      int t = PulseLen(pulse[i]);
      if (t >= 10)
      {
        i++;
        break;
      }
    }

    start = i;

    // Do the actual decoding.
    length = 0;
    uint8_t bits = 0;
    uint8_t bit = 1;
    for (; i < pulse.GetSize() - 1 && bytes.GetSize() < 16; i += 2)
    {
      int p0 = PulseLen(pulse[i]);
      int p1 = PulseLen(pulse[i + 1]);
      if (p0 == 3 && p1 == 1)
      {
        // 1 bit
        bits |= bit;
      }
      else if (p0 == 1 && p1 == 3)
      {
        // 0 bit
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
      bit <<= 1;

      if ((length & 7) == 0)
      {
        bytes.Add(bits);
        bits = 0;
        bit = 1;
      }
    }

    // Include a last incomplete byte, if any.
    if (bits != 0)
    {
      bytes.Add(bits);
    }

    return true;
  }

  bool BytesToPulse(const CArray<uint8_t>& bytes, int length, CArray<uint16_t>& pulse)
  {
    pulse.Add(PulseDuration(1));
    pulse.Add(PulseDuration(30));

    for (int i = 0; i < length; i++)
    {
      uint8_t mask = 1 << (i & 7);
      bool bit = (bytes[i / 8] & mask) != 0;
      if (bit)
      {
        pulse.Add(PulseDuration(3));
        pulse.Add(PulseDuration(1));
      } else
      {
        pulse.Add(PulseDuration(1));
        pulse.Add(PulseDuration(3));
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

    if (length == 24) {
      sprintf(desc, "%d bits (%d): <%06x>", length, start, (int)d0);
    } else {
      sprintf(desc, "%d bits (%d): <%08x %08x>", length, start, (int)d0, (int)d1);
    }
  }

  virtual const char* GetString(int i) override { return nullptr; }
};
