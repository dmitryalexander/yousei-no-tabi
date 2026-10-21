#include "fast/Prewarm.h"
#include <cassert>
#include <future>
#include <iostream>
int main() {
    using namespace Fast::Prewarm;
    const auto a=Key(123,0xffffull<<17,false,true);
    const auto b=Key(123,~0ull<<17,false,true);
    assert(a==b);const auto epoch=sourceEpoch.load();
    RememberSource(a,"generated HLSL",37,epoch);
    auto prepared=FindSource(b);assert(prepared&&prepared->text=="generated HLSL"&&prepared->floats==37);
    assert(!FindSource(Key(123,0xffffull<<17,true,true)));
    TrimSources({});assert(!FindSource(a));
    ClearSources();RememberSource(a,"stale",1,epoch);assert(!FindSource(a));
    std::vector<std::future<void>> tasks;
    for(int i=0;i<12;++i)tasks.push_back(std::async(std::launch::async,[i] {
        const auto key=Key(i,123,false,false);
        for(int n=0;n<100;++n) {RememberSource(key,"immutable",9,sourceEpoch.load());auto value=FindSource(key);
            assert(value&&value->text=="immutable"&&value->floats==9);}
    }));
    for(auto& task:tasks)task.get();
    ClearSources();for(int i=0;i<18;++i)RememberSource(Key(i,123,false,false),std::string(1024*1024,'x'),4,sourceEpoch.load());
    assert(sourceBytes<=16*1024*1024&&sources.size()==16);
    std::cout<<"{\"passed\":true,\"canonical_default_shader_key\":true,\"settings_invalidation\":true,\"trim_unused\":true,\"epoch_rejects_stale_work\":true,\"concurrent_roundtrips\":1200,\"bounded_bytes\":"<<sourceBytes<<"}\n";
}
