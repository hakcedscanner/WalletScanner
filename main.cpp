#include "ScanWallet.hpp"

int main(int argc, char const *argv[])
{
    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini("config.ini", pt);

    ScanWallet app(pt);
    app.run();

    return 0;
}
