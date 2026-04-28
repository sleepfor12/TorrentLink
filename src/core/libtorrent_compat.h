#ifndef PFD_CORE_LIBTORRENT_COMPAT_H
#define PFD_CORE_LIBTORRENT_COMPAT_H

#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/error_code.hpp>
#include <libtorrent/magnet_uri.hpp>

#include <functional>
#include <string>

namespace pfd::core::ltcompat {

inline libtorrent::add_torrent_params parseMagnetUri(const std::string& uri,
                                                     libtorrent::error_code& ec) {
  return libtorrent::parse_magnet_uri(uri, ec);
}

inline int pieceIndexToInt(libtorrent::piece_index_t piece) {
  return static_cast<int>(static_cast<libtorrent::piece_index_t::underlying_type>(piece));
}

inline void setPieceHashes(libtorrent::create_torrent& torrent, const std::string& rootPath,
                           const std::function<void(int current, int total)>& onPieceDone,
                           libtorrent::error_code& ec) {
  const int total = torrent.num_pieces();
  libtorrent::set_piece_hashes(
      torrent, rootPath,
      [onPieceDone, total](libtorrent::piece_index_t piece) {
        if (onPieceDone) {
          onPieceDone(pieceIndexToInt(piece) + 1, total);
        }
      },
      ec);
}

}  // namespace pfd::core::ltcompat

#endif  // PFD_CORE_LIBTORRENT_COMPAT_H
