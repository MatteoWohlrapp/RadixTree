

// ((PAGE_SIZE-16)/2)/4 and (PAGE_SIZE-20)/4 must both be integers
// 48 works for a degree of 4 in the inner nodes, 7 in the outer nodes
namespace Configuration
{
    constexpr int page_size = 52;
}
