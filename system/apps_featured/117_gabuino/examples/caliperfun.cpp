#include <library.h>

using namespace BIOS;
using namespace BIOS::GPIO;

int ReceiveWord(uint32_t& data)
{
    int counter;
    data = 0;
    for (int i=0; i<23; i++)
    {
        // low: 6us
        counter = 0;
        while (!GPIO::DigitalRead(P2) && counter < 10)
           counter ++; 
        if (2 > counter || counter > 5)
            return i*1000 + 100 + counter;
    
        // high: 6us (sample in middle)
        counter = 2;
        GPIO::DigitalRead(P2);
        GPIO::DigitalRead(P2);
        
        data >>= 1;
        data |= GPIO::DigitalRead(P1) ? 0 : 1 << 22;
    
        while (GPIO::DigitalRead(P2) && counter < 10)
            counter ++;
        if (2 > counter || counter > 5)
            return i*1000 + 500 + counter;
    }
    
    if (data & (1<<21))
        data |= ~((1<<22)-1);

    counter = 0;
    while (!GPIO::DigitalRead(P2) && counter < 10)
       counter ++; 
    if (2 > counter || counter > 5)
        return 40000 + counter;
    
    return 0;
}

int ReceivePacket(uint32_t& data1, uint32_t& data2)
{
    // 50us
    int counter = 0;
    while (GPIO::DigitalRead(P2) && counter < 100)
       counter ++; 
    if (34-10 > counter || counter > 34+10)
        return false;
        
    if (ReceiveWord(data1) != 0)
        return false;
        
    // 100us
    counter = 0;
    while (GPIO::DigitalRead(P2) && counter < 600)
        counter ++; 
    if (76-10 > counter || counter > 76+10)
        return false;
        
    if (ReceiveWord(data2) != 0)
        return false;

    // 60us
    counter = 0;
    while (GPIO::DigitalRead(P2) && counter < 600)
        counter ++; 
    if (41-10 > counter || counter > 41+10)
        return false;

    return true;
}

int main(void)
{
    BIOS::DBG::Print(R"(
<div id='measurement' style='font-size:20px;'></div>
<br><div id='bar' style='background:gray; width:200px; height:16px;'></div>
<script>
ticksGood = 0;
metric = [{t:"M4",d:6.9,c:0}, {t:"M5",d:7.8,c:0}, {t:"M6",d:9.9,c:0},
  {t:"M8",d:12.8,c:0}];

window.onMeas = (len, beep) =>
{
    var mm = len/1024*2.54;
    var found = metric.find(m=>(mm >= m.d-0.2 && mm <= m.d+0.2));
    var table = "<table border=1 width=200>";
    table += "<thead><tr><th>Size</th><th>Count</th></tr></thead>"
    for (var i in metric)
        table += "<tr><td>"+metric[i].t+"</td><td>"+metric[i].c+"</td></tr>";
    table += "</table>";

    $('#measurement').html(mm.toFixed(1) + "mm " +
        (found ? found.t : "") + "<br>" +
        table);
    
    $('#bar').width(len/100);
    $('#bar').css("background", found ? "green" : "gray");
    if (found)
    {
        if (ticksGood++ == 5)
        {
            BIOS.memWrite(beep, [1]);
            found.c++;
        }
    }
    else
        ticksGood = 0;
    
}
</script>)");

    PinMode(EPin::P1, EMode::Input);
    PinMode(EPin::P2, EMode::Input);
    PinMode(EPin::P3, EMode::Output);
    DigitalWrite(EPin::P3, 1);
    bool beep = false;
    BIOS::KEY::EKey key;
    while ((key = BIOS::KEY::GetKey()) != BIOS::KEY::EKey::Escape)
    {
        if (beep)
        {
            BIOS::SYS::Beep(200);
            beep = false;
        }
        if (GPIO::DigitalRead(P2))
        {
            OS::DisableInterrupts();
            uint32_t data1, data2;
            int valid = ReceivePacket(data1, data2);
            OS::EnableInterrupts(0);
            
            if (valid)
                BIOS::DBG::Print("<script>window.onMeas(%d, 0x%08x);</script>", data2, &beep); 
        }
    }
    return 0;
}
