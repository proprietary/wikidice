{ lib
, llvmPackages_17
, boost183
, cmake
, rocksdb
, python311Packages.python
}:

llvmPackages_17.libcxxStdenv.mkDerivation rec {
  pname = "wikidice";
  version = "0.2.0";
  
  src = ./.;

  nativeBuildInputs = [ cmake ];
  buildInputs = [
    boost183
    rocksdb
    llvmPackages_17.libcxx
  ];

  cmakeFlags = [
    "-DENABLE_TESTING=OFF"
    "-DENABLE_INSTALL=ON"
  ];
}
