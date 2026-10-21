#include "fast/PresentationRate.h"
#include "fast/MaterialRelief.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    Fast::PresentationRate meter;
    double now = 0;
    for (int fps : {60, 15, 7, 120, 7, 30}) {
        assert(meter.Rate(fps, 3, false) == 0); // no old-target number
        meter.Record(now, fps, 3, false);
        assert(meter.Rate(fps, 3, false) == 0);
        for (int frame=0;frame<fps*2;frame++) {
            now += 1.0/fps;
            meter.Record(now, fps, 3, false);
            assert(std::abs(meter.Rate(fps,3,false)-fps)<0.0001);
        }
    }
    // Report real slowdowns, never clamp or replace measured FPS with the target.
    for (int i=0;i<12;i++) {now+=.2;meter.Record(now,30,3,false);}
    assert(std::abs(meter.Rate(30,3,false)-5)<0.0001);
    assert(meter.Rate(30,2,false)==0 && meter.Rate(30,3,true)==0);
    meter.Record(now+.2,30,3,true);
    assert(meter.Rate(30,3,true)==0);
    meter.Record(now+.4,30,3,true);
    assert(std::abs(meter.Rate(30,3,true)-5)<0.0001);
    meter.Record(0,30,3,true); // clock discontinuity
    assert(meter.Rate(30,3,true)==0);
    assert(Fast::KokiriGrassRelief("alt/scenes/shared/spot04_scene/spot04_sceneTex_00FE18"));
    assert(Fast::KokiriGrassRelief("alt/scenes/shared/spot04_scene/spot04_sceneTex_010618"));
    assert(!Fast::KokiriGrassRelief("alt/scenes/shared/spot04_scene/spot04_room_0Tex_00F508"));
    assert(!Fast::KokiriGrassRelief("alt/scenes/shared/spot00_scene/spot00_sceneTex_01AB98"));
    assert(!Fast::KokiriGrassRelief("scenes/shared/spot04_scene/spot04_sceneTex_00FE18"));
    std::cout << "PASS: target/graphics/vsync resets, rapid low-FPS settling, honest slowdown reporting, grass scope\n";
}
