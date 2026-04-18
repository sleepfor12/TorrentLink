#ifndef PFD_LT_ADD_TORRENT_TORRENT_INFO_PTR_H
#define PFD_LT_ADD_TORRENT_TORRENT_INFO_PTR_H

#include <libtorrent/add_torrent_params.hpp>

#include <memory>
#include <type_traits>
#include <utility>

namespace pfd::lt {

// Matches libtorrent::add_torrent_params::ti (may be libtorrent::torrent_info or v2::torrent_info).
using AddTorrentTorrentInfoPtr = decltype(std::declval<libtorrent::add_torrent_params>().ti);

using AddTorrentTorrentInfoConstPtr = std::shared_ptr<
    const std::remove_const_t<typename AddTorrentTorrentInfoPtr::element_type>>;

template <class... Args>
inline AddTorrentTorrentInfoPtr make_add_torrent_torrent_info(Args&&... args) {
  using Raw = std::remove_const_t<typename AddTorrentTorrentInfoPtr::element_type>;
  return std::make_shared<Raw>(std::forward<Args>(args)...);
}

}  // namespace pfd::lt

#endif
