#include "tweeta.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const Route ROUTES[] = {
  {"core","emojis","GET","/api/emojis",0,PAYLOAD_QUERY,NULL},
  {"core","transparency","GET","/api/transparency/:user",1,PAYLOAD_QUERY,NULL},
  {"core","transparency-asn","GET","/api/transparency/:user/asn",1,PAYLOAD_QUERY,NULL},
  {"core","owoembed","GET","/api/owoembed",0,PAYLOAD_QUERY,NULL},
  {"core","ack-warning","POST","/api/warning/acknowledge",0,PAYLOAD_BODY,NULL},

  {"auth","cap-config","GET","/api/auth/cap/config",0,PAYLOAD_QUERY,NULL},
  {"auth","cap-rate-limit-bypass","POST","/api/auth/cap/rate-limit-bypass",0,PAYLOAD_BODY,NULL},
  {"auth","me","GET","/api/auth/me",0,PAYLOAD_QUERY,NULL},
  {"auth","switch-to-delegate","POST","/api/auth/switch-to-delegate",0,PAYLOAD_BODY,NULL},
  {"auth","switch-to-primary","POST","/api/auth/switch-to-primary",0,PAYLOAD_BODY,NULL},
  {"auth","add-account","POST","/api/auth/add-account",0,PAYLOAD_BODY,NULL},
  {"auth","username-availability","GET","/api/auth/username-availability",0,PAYLOAD_QUERY,NULL},
  {"auth","generate-registration-options","POST","/api/auth/generate-registration-options",0,PAYLOAD_BODY,NULL},
  {"auth","verify-registration","POST","/api/auth/verify-registration",0,PAYLOAD_BODY,NULL},
  {"auth","generate-authentication-options","POST","/api/auth/generate-authentication-options",0,PAYLOAD_BODY,NULL},
  {"auth","verify-authentication","POST","/api/auth/verify-authentication",0,PAYLOAD_BODY,NULL},
  {"auth","passkeys","GET","/api/auth/passkeys",0,PAYLOAD_QUERY,NULL},
  {"auth","rename-passkey","PUT","/api/auth/passkeys/:credId/name",1,PAYLOAD_BODY,NULL},
  {"auth","delete-passkey","DELETE","/api/auth/passkeys/:credId",1,PAYLOAD_NONE,NULL},
  {"auth","register-password","POST","/api/auth/register-with-password",0,PAYLOAD_BODY,NULL},
  {"auth","basic-login","POST","/api/auth/basic-login",0,PAYLOAD_BODY,NULL},
  {"auth","moderation-history","GET","/api/auth/moderation-history",0,PAYLOAD_QUERY,NULL},
  {"auth","validate-accounts","POST","/api/auth/validate-accounts",0,PAYLOAD_BODY,NULL},

  {"tweets","create","POST","/api/tweets/",0,PAYLOAD_BODY,NULL},
  {"tweets","react","POST","/api/tweets/:id/reaction",1,PAYLOAD_BODY,NULL},
  {"tweets","reactions","GET","/api/tweets/:id/reactions",1,PAYLOAD_QUERY,NULL},
  {"tweets","get","GET","/api/tweets/:id",1,PAYLOAD_QUERY,NULL},
  {"tweets","like","POST","/api/tweets/:id/like",1,PAYLOAD_BODY,NULL},
  {"tweets","retweet","POST","/api/tweets/:id/retweet",1,PAYLOAD_BODY,NULL},
  {"tweets","poll-vote","POST","/api/tweets/:id/poll/vote",1,PAYLOAD_BODY,NULL},
  {"tweets","likes","GET","/api/tweets/:id/likes",1,PAYLOAD_QUERY,NULL},
  {"tweets","retweets","GET","/api/tweets/:id/retweets",1,PAYLOAD_QUERY,NULL},
  {"tweets","quotes","GET","/api/tweets/:id/quotes",1,PAYLOAD_QUERY,NULL},
  {"tweets","can-reply","GET","/api/tweets/can-reply/:id",1,PAYLOAD_QUERY,NULL},
  {"tweets","bulk-delete","POST","/api/tweets/bulk-delete",0,PAYLOAD_BODY,NULL},
  {"tweets","delete","DELETE","/api/tweets/:id",1,PAYLOAD_NONE,NULL},
  {"tweets","reply-restriction","PATCH","/api/tweets/:id/reply-restriction",1,PAYLOAD_BODY,NULL},
  {"tweets","update","PUT","/api/tweets/:id",1,PAYLOAD_BODY,NULL},
  {"tweets","outline","PATCH","/api/tweets/:id/outline",1,PAYLOAD_BODY,NULL},
  {"tweets","edit-history","GET","/api/tweets/:id/edit-history",1,PAYLOAD_QUERY,NULL},

  {"timeline","home","GET","/api/timeline/",0,PAYLOAD_QUERY,NULL},
  {"timeline","following","GET","/api/timeline/following",0,PAYLOAD_QUERY,NULL},
  {"timeline","interests","GET","/api/timeline/for-you/interests",0,PAYLOAD_QUERY,NULL},
  {"timeline","clear-interests","DELETE","/api/timeline/for-you/interests",0,PAYLOAD_NONE,NULL},
  {"timeline","delete-interest","DELETE","/api/timeline/for-you/interests/:topic",1,PAYLOAD_NONE,NULL},

  {"profile","get","GET","/api/profile/:username",1,PAYLOAD_QUERY,NULL},
  {"profile","replies","GET","/api/profile/:username/replies",1,PAYLOAD_QUERY,NULL},
  {"profile","media","GET","/api/profile/:username/media",1,PAYLOAD_QUERY,NULL},
  {"profile","posts","GET","/api/profile/:username/posts",1,PAYLOAD_QUERY,NULL},
  {"profile","highlights","GET","/api/profile/:username/highlights",1,PAYLOAD_QUERY,NULL},
  {"profile","highlight","POST","/api/profile/:username/highlights/:postId",2,PAYLOAD_BODY,NULL},
  {"profile","update","PUT","/api/profile/:username",1,PAYLOAD_BODY,NULL},
  {"profile","follow","POST","/api/profile/:username/follow",1,PAYLOAD_BODY,NULL},
  {"profile","unfollow","DELETE","/api/profile/:username/follow",1,PAYLOAD_NONE,NULL},
  {"profile","notify-tweets","POST","/api/profile/:username/notify-tweets",1,PAYLOAD_BODY,NULL},
  {"profile","set-avatar","POST","/api/profile/:username/avatar",1,PAYLOAD_FILE,"avatar"},
  {"profile","delete-avatar","DELETE","/api/profile/:username/avatar",1,PAYLOAD_NONE,NULL},
  {"profile","set-banner","POST","/api/profile/:username/banner",1,PAYLOAD_FILE,"banner"},
  {"profile","delete-banner","DELETE","/api/profile/:username/banner",1,PAYLOAD_NONE,NULL},
  {"profile","followers","GET","/api/profile/:username/followers",1,PAYLOAD_QUERY,NULL},
  {"profile","following","GET","/api/profile/:username/following",1,PAYLOAD_QUERY,NULL},
  {"profile","mutuals","GET","/api/profile/:username/mutuals",1,PAYLOAD_QUERY,NULL},
  {"profile","followers-you-know","GET","/api/profile/:username/followers-you-know",1,PAYLOAD_QUERY,NULL},
  {"profile","change-username","PATCH","/api/profile/:username/username",1,PAYLOAD_BODY,NULL},
  {"profile","change-password","PATCH","/api/profile/:username/password",1,PAYLOAD_BODY,NULL},
  {"profile","outlines","PATCH","/api/profile/:username/outlines",1,PAYLOAD_BODY,NULL},
  {"profile","delete","DELETE","/api/profile/:username",1,PAYLOAD_BODY,NULL},
  {"profile","set-password","POST","/api/profile/:username/password",1,PAYLOAD_BODY,NULL},
  {"profile","follow-requests","GET","/api/profile/follow-requests",0,PAYLOAD_QUERY,NULL},
  {"profile","approve-follow-request","POST","/api/profile/follow-requests/:requestId/approve",1,PAYLOAD_BODY,NULL},
  {"profile","deny-follow-request","POST","/api/profile/follow-requests/:requestId/deny",1,PAYLOAD_BODY,NULL},
  {"profile","affiliate","POST","/api/profile/:username/affiliate",1,PAYLOAD_BODY,NULL},
  {"profile","affiliate-requests","GET","/api/profile/affiliate-requests",0,PAYLOAD_QUERY,NULL},
  {"profile","approve-affiliate-request","POST","/api/profile/affiliate-requests/:requestId/approve",1,PAYLOAD_BODY,NULL},
  {"profile","deny-affiliate-request","POST","/api/profile/affiliate-requests/:requestId/deny",1,PAYLOAD_BODY,NULL},
  {"profile","affiliates","GET","/api/profile/:username/affiliates",1,PAYLOAD_QUERY,NULL},
  {"profile","remove-affiliate","DELETE","/api/profile/remove-affiliate",0,PAYLOAD_NONE,NULL},
  {"profile","pin","POST","/api/profile/pin/:tweetId",1,PAYLOAD_BODY,NULL},
  {"profile","unpin","DELETE","/api/profile/pin/:tweetId",1,PAYLOAD_NONE,NULL},
  {"profile","pin-for-user","POST","/api/profile/:username/pin/:tweetId",2,PAYLOAD_BODY,NULL},
  {"profile","unpin-for-user","DELETE","/api/profile/:username/pin/:tweetId",2,PAYLOAD_NONE,NULL},
  {"profile","private","POST","/api/profile/settings/private",0,PAYLOAD_BODY,NULL},
  {"profile","community-tag","POST","/api/profile/settings/community-tag",0,PAYLOAD_BODY,NULL},
  {"profile","transparency-location","POST","/api/profile/settings/transparency-location",0,PAYLOAD_BODY,NULL},
  {"profile","algorithm-stats","GET","/api/profile/:username/algorithm-stats",1,PAYLOAD_QUERY,NULL},
  {"profile","spam-score","GET","/api/profile/:username/spam-score",1,PAYLOAD_QUERY,NULL},

  {"bookmarks","add","POST","/api/bookmarks/add",0,PAYLOAD_BODY,NULL},
  {"bookmarks","remove","POST","/api/bookmarks/remove",0,PAYLOAD_BODY,NULL},
  {"bookmarks","list","GET","/api/bookmarks/",0,PAYLOAD_QUERY,NULL},
  {"blocking","block","POST","/api/blocking/block",0,PAYLOAD_BODY,NULL},
  {"blocking","unblock","POST","/api/blocking/unblock",0,PAYLOAD_BODY,NULL},
  {"blocking","check","GET","/api/blocking/check/:userId",1,PAYLOAD_QUERY,NULL},
  {"blocking","mute","POST","/api/blocking/mute",0,PAYLOAD_BODY,NULL},
  {"blocking","unmute","POST","/api/blocking/unmute",0,PAYLOAD_BODY,NULL},
  {"blocking","check-mute","GET","/api/blocking/check-mute/:userId",1,PAYLOAD_QUERY,NULL},
  {"blocking","causes","GET","/api/blocking/causes",0,PAYLOAD_QUERY,NULL},
  {"muted","words","GET","/api/muted/words",0,PAYLOAD_QUERY,NULL},
  {"muted","add-word","POST","/api/muted/words",0,PAYLOAD_BODY,NULL},
  {"muted","delete-word","DELETE","/api/muted/words/:id",1,PAYLOAD_NONE,NULL},
  {"muted","delete-word-by-word","DELETE","/api/muted/words/by-word/:word",1,PAYLOAD_NONE,NULL},
  {"muted","mute-conversation","POST","/api/muted/conversations/:postId",1,PAYLOAD_BODY,NULL},
  {"muted","conversations","GET","/api/muted/conversations",0,PAYLOAD_QUERY,NULL},
  {"muted","conversation-status","GET","/api/muted/conversations/:postId/status",1,PAYLOAD_QUERY,NULL},

  {"communities","create","POST","/api/communities",0,PAYLOAD_BODY,NULL},
  {"communities","get","GET","/api/communities/:id",1,PAYLOAD_QUERY,NULL},
  {"communities","list","GET","/api/communities",0,PAYLOAD_QUERY,NULL},
  {"communities","mine","GET","/api/communities/user/me",0,PAYLOAD_QUERY,NULL},
  {"communities","update","PATCH","/api/communities/:id",1,PAYLOAD_BODY,NULL},
  {"communities","delete","DELETE","/api/communities/:id",1,PAYLOAD_NONE,NULL},
  {"communities","join","POST","/api/communities/:id/join",1,PAYLOAD_BODY,NULL},
  {"communities","leave","POST","/api/communities/:id/leave",1,PAYLOAD_BODY,NULL},
  {"communities","members","GET","/api/communities/:id/members",1,PAYLOAD_QUERY,NULL},
  {"communities","set-role","POST","/api/communities/:id/members/:userId/role",2,PAYLOAD_BODY,NULL},
  {"communities","ban","POST","/api/communities/:id/members/:userId/ban",2,PAYLOAD_BODY,NULL},
  {"communities","unban","POST","/api/communities/:id/members/:userId/unban",2,PAYLOAD_BODY,NULL},
  {"communities","access-mode","PATCH","/api/communities/:id/access-mode",1,PAYLOAD_BODY,NULL},
  {"communities","join-requests","GET","/api/communities/:id/join-requests",1,PAYLOAD_QUERY,NULL},
  {"communities","approve-join-request","POST","/api/communities/:id/join-requests/:requestId/approve",2,PAYLOAD_BODY,NULL},
  {"communities","reject-join-request","POST","/api/communities/:id/join-requests/:requestId/reject",2,PAYLOAD_BODY,NULL},
  {"communities","for-user","GET","/api/users/:userId/communities",1,PAYLOAD_QUERY,NULL},
  {"communities","icon","POST","/api/communities/:id/icon",1,PAYLOAD_FILE,"icon"},
  {"communities","banner","POST","/api/communities/:id/banner",1,PAYLOAD_FILE,"banner"},
  {"communities","tweets","GET","/api/communities/:id/tweets",1,PAYLOAD_QUERY,NULL},
  {"communities","tag","PATCH","/api/communities/:id/tag",1,PAYLOAD_BODY,NULL},
  {"communities","pin-tweet","POST","/api/communities/:id/tweets/:tweetId/pin",2,PAYLOAD_BODY,NULL},
  {"communities","unpin-tweet","POST","/api/communities/:id/tweets/:tweetId/unpin",2,PAYLOAD_BODY,NULL},
  {"communities","search","GET","/api/communities/search",0,PAYLOAD_QUERY,NULL},
  {"communities","trending","GET","/api/communities/trending",0,PAYLOAD_QUERY,NULL},
  {"communities","recommended","GET","/api/communities/recommended",0,PAYLOAD_QUERY,NULL},
  {"communities","mod-log","GET","/api/communities/:id/mod-log",1,PAYLOAD_QUERY,NULL},
  {"communities","create-invite","POST","/api/communities/:id/invites",1,PAYLOAD_BODY,NULL},
  {"communities","accept-invite","POST","/api/communities/invites/:token/accept",1,PAYLOAD_BODY,NULL},
  {"communities","invites","GET","/api/communities/:id/invites",1,PAYLOAD_QUERY,NULL},
  {"communities","delete-invite","DELETE","/api/communities/:id/invites/:inviteId",2,PAYLOAD_NONE,NULL},

  {"delegates","invite","POST","/api/delegates/invite",0,PAYLOAD_BODY,NULL},
  {"delegates","accept","POST","/api/delegates/:id/accept",1,PAYLOAD_BODY,NULL},
  {"delegates","decline","POST","/api/delegates/:id/decline",1,PAYLOAD_BODY,NULL},
  {"delegates","delete","DELETE","/api/delegates/:id",1,PAYLOAD_NONE,NULL},
  {"delegates","summary","GET","/api/delegates/summary",0,PAYLOAD_QUERY,NULL},
  {"delegates","my-delegates","GET","/api/delegates/my-delegates",0,PAYLOAD_QUERY,NULL},
  {"delegates","my-delegations","GET","/api/delegates/my-delegations",0,PAYLOAD_QUERY,NULL},
  {"delegates","pending-invitations","GET","/api/delegates/pending-invitations",0,PAYLOAD_QUERY,NULL},
  {"delegates","sent-invitations","GET","/api/delegates/sent-invitations",0,PAYLOAD_QUERY,NULL},
  {"delegates","check","GET","/api/delegates/check/:userId",1,PAYLOAD_QUERY,NULL},

  {"dm","conversations","GET","/api/dm/conversations",0,PAYLOAD_QUERY,NULL},
  {"dm","conversation","GET","/api/dm/conversations/:id",1,PAYLOAD_QUERY,NULL},
  {"dm","create-conversation","POST","/api/dm/conversations",0,PAYLOAD_BODY,NULL},
  {"dm","send","POST","/api/dm/conversations/:id/messages",1,PAYLOAD_BODY,NULL},
  {"dm","read","PATCH","/api/dm/conversations/:id/read",1,PAYLOAD_BODY,NULL},
  {"dm","add-participant","POST","/api/dm/conversations/:id/participants",1,PAYLOAD_BODY,NULL},
  {"dm","remove-participant","DELETE","/api/dm/conversations/:id/participants/:userId",2,PAYLOAD_NONE,NULL},
  {"dm","title","PATCH","/api/dm/conversations/:id/title",1,PAYLOAD_BODY,NULL},
  {"dm","react","POST","/api/dm/messages/:messageId/reactions",1,PAYLOAD_BODY,NULL},
  {"dm","typing","POST","/api/dm/conversations/:id/typing",1,PAYLOAD_BODY,NULL},
  {"dm","typing-stop","POST","/api/dm/conversations/:id/typing-stop",1,PAYLOAD_BODY,NULL},
  {"dm","edit","PUT","/api/dm/messages/:messageId",1,PAYLOAD_BODY,NULL},
  {"dm","delete-message","DELETE","/api/dm/messages/:messageId",1,PAYLOAD_NONE,NULL},
  {"dm","disappearing","PATCH","/api/dm/conversations/:id/disappearing",1,PAYLOAD_BODY,NULL},
  {"dm","invite","POST","/api/dm/conversations/:id/invite",1,PAYLOAD_BODY,NULL},
  {"dm","revoke-invite","POST","/api/dm/conversations/:id/invite/revoke",1,PAYLOAD_BODY,NULL},
  {"dm","join","POST","/api/dm/conversations/join/:token",1,PAYLOAD_BODY,NULL},
  {"dm","permissions","PATCH","/api/dm/conversations/:id/permissions",1,PAYLOAD_BODY,NULL},
  {"dm","participant-role","PATCH","/api/dm/conversations/:id/participants/:userId/role",2,PAYLOAD_BODY,NULL},
  {"dm","pin","POST","/api/dm/conversations/:id/pin/:messageId",2,PAYLOAD_BODY,NULL},
  {"dm","unpin","DELETE","/api/dm/conversations/:id/pin/:messageId",2,PAYLOAD_NONE,NULL},
  {"dm","pinned","GET","/api/dm/conversations/:id/pinned",1,PAYLOAD_QUERY,NULL},

  {"notifications","list","GET","/api/notifications/",0,PAYLOAD_QUERY,NULL},
  {"notifications","unread-count","GET","/api/notifications/unread-count",0,PAYLOAD_QUERY,NULL},
  {"notifications","read","PATCH","/api/notifications/:id/read",1,PAYLOAD_BODY,NULL},
  {"notifications","mark-all-read","PATCH","/api/notifications/mark-all-read",0,PAYLOAD_BODY,NULL},
  {"push","vapid-key","GET","/api/push/vapid-key",0,PAYLOAD_QUERY,NULL},
  {"push","subscribe","POST","/api/push/subscribe",0,PAYLOAD_BODY,NULL},
  {"push","unsubscribe","POST","/api/push/unsubscribe",0,PAYLOAD_BODY,NULL},
  {"push","status","GET","/api/push/status",0,PAYLOAD_QUERY,NULL},

  {"lists","list","GET","/api/lists/",0,PAYLOAD_QUERY,NULL},
  {"lists","for-user","GET","/api/lists/user/:username",1,PAYLOAD_QUERY,NULL},
  {"lists","containing","GET","/api/lists/containing/:username",1,PAYLOAD_QUERY,NULL},
  {"lists","create","POST","/api/lists/",0,PAYLOAD_BODY,NULL},
  {"lists","get","GET","/api/lists/:id",1,PAYLOAD_QUERY,NULL},
  {"lists","update","PATCH","/api/lists/:id",1,PAYLOAD_BODY,NULL},
  {"lists","delete","DELETE","/api/lists/:id",1,PAYLOAD_NONE,NULL},
  {"lists","tweets","GET","/api/lists/:id/tweets",1,PAYLOAD_QUERY,NULL},
  {"lists","add-member","POST","/api/lists/:id/members",1,PAYLOAD_BODY,NULL},
  {"lists","remove-member","DELETE","/api/lists/:id/members/:userId",2,PAYLOAD_NONE,NULL},
  {"lists","follow","POST","/api/lists/:id/follow",1,PAYLOAD_BODY,NULL},
  {"lists","unfollow","DELETE","/api/lists/:id/follow",1,PAYLOAD_NONE,NULL},
  {"lists","members","GET","/api/lists/:id/members",1,PAYLOAD_QUERY,NULL},
  {"lists","followers","GET","/api/lists/:id/followers",1,PAYLOAD_QUERY,NULL},

  {"articles","create","POST","/api/articles/",0,PAYLOAD_BODY,NULL},
  {"articles","list","GET","/api/articles/",0,PAYLOAD_QUERY,NULL},
  {"articles","get","GET","/api/articles/:id",1,PAYLOAD_QUERY,NULL},
  {"search","users","GET","/api/search/users",0,PAYLOAD_QUERY,NULL},
  {"search","posts","GET","/api/search/posts",0,PAYLOAD_QUERY,NULL},
  {"public-tweets","list","GET","/api/public-tweets/",0,PAYLOAD_QUERY,NULL},
  {"scheduled","create","POST","/api/scheduled/",0,PAYLOAD_BODY,NULL},
  {"scheduled","list","GET","/api/scheduled/",0,PAYLOAD_QUERY,NULL},
  {"scheduled","delete","DELETE","/api/scheduled/:id",1,PAYLOAD_NONE,NULL},
  {"reports","create","POST","/api/reports/create",0,PAYLOAD_BODY,NULL},
  {"translate","translate","POST","/api/translate/",0,PAYLOAD_BODY,NULL},
  {"trends","list","GET","/api/trends/",0,PAYLOAD_QUERY,NULL},
  {"tenor","search","GET","/api/tenor/search",0,PAYLOAD_QUERY,NULL},
  {"unsplash","search","GET","/api/unsplash/search",0,PAYLOAD_QUERY,NULL},
  {"sse","connect","GET","/api/sse",0,PAYLOAD_QUERY,NULL},
  {"upload","media","POST","/api/upload",0,PAYLOAD_FILE,"file"},
  {"upload","get","GET","/api/uploads/:filename",1,PAYLOAD_QUERY,NULL},

  {"explore","best-of-week","GET","/api/explore/best-of-week",0,PAYLOAD_QUERY,NULL},
  {"explore","trending-users","GET","/api/explore/trending-users",0,PAYLOAD_QUERY,NULL},
  {"explore","most-bookmarked","GET","/api/explore/most-bookmarked",0,PAYLOAD_QUERY,NULL},
  {"explore","most-discussed","GET","/api/explore/most-discussed",0,PAYLOAD_QUERY,NULL},
  {"explore","longest-threads","GET","/api/explore/longest-threads",0,PAYLOAD_QUERY,NULL},
  {"explore","with-media","GET","/api/explore/with-media",0,PAYLOAD_QUERY,NULL},
  {"explore","with-polls","GET","/api/explore/with-polls",0,PAYLOAD_QUERY,NULL},
  {"explore","digest","GET","/api/explore/digest",0,PAYLOAD_QUERY,NULL},
  {"explore","top-hashtags","GET","/api/explore/top-hashtags",0,PAYLOAD_QUERY,NULL},
  {"explore","leaderboard","GET","/api/explore/leaderboard",0,PAYLOAD_QUERY,NULL},
  {"explore","suggested-users","GET","/api/explore/suggested-users",0,PAYLOAD_QUERY,NULL},
  {"explore","directory","GET","/api/explore/directory",0,PAYLOAD_QUERY,NULL},
  {"explore","user-analytics","GET","/api/explore/users/:username/analytics",1,PAYLOAD_QUERY,NULL},
  {"explore","common-followers","GET","/api/explore/users/:username/common-followers",1,PAYLOAD_QUERY,NULL},
  {"explore","top-posts","GET","/api/explore/users/:username/top-posts",1,PAYLOAD_QUERY,NULL},
  {"explore","stats","GET","/api/explore/stats",0,PAYLOAD_QUERY,NULL},

  {"mpi","send-create","POST","/api/mpi/send/create",0,PAYLOAD_BODY,NULL},
  {"mpi","send-confirm","POST","/api/mpi/send/confirm",0,PAYLOAD_BODY,NULL},
  {"mpi","request","POST","/api/mpi/request",0,PAYLOAD_BODY,NULL},
  {"mpi","request-pay","POST","/api/mpi/request/:messageId/pay",1,PAYLOAD_BODY,NULL},
  {"mpi","request-confirm","POST","/api/mpi/request/:messageId/confirm",1,PAYLOAD_BODY,NULL},
  {"mpi","payments-by-message","GET","/api/mpi/payments/by-message/:messageId",1,PAYLOAD_QUERY,NULL},
  {"shop","user","GET","/api/mpi/shop/user/:username",1,PAYLOAD_QUERY,NULL},
  {"shop","product","GET","/api/mpi/shop/product/:id",1,PAYLOAD_QUERY,NULL},
  {"shop","create-product","POST","/api/mpi/shop/product",0,PAYLOAD_BODY,NULL},
  {"shop","update-product","PATCH","/api/mpi/shop/product/:id",1,PAYLOAD_BODY,NULL},
  {"shop","delete-product","DELETE","/api/mpi/shop/product/:id",1,PAYLOAD_NONE,NULL},
  {"shop","buy","POST","/api/mpi/shop/product/:id/buy",1,PAYLOAD_BODY,NULL},
  {"shop","confirm","POST","/api/mpi/shop/product/:id/confirm",1,PAYLOAD_BODY,NULL},
  {"shop","purchase","GET","/api/mpi/shop/purchase/:id",1,PAYLOAD_QUERY,NULL},

  {"admin","stats","GET","/api/admin/stats",0,PAYLOAD_QUERY,NULL},
  {"admin","users","GET","/api/admin/users",0,PAYLOAD_QUERY,NULL},
  {"admin","create-user","POST","/api/admin/users",0,PAYLOAD_BODY,NULL},
  {"admin","create-affiliate-request","POST","/api/admin/users/:id/affiliate-requests",1,PAYLOAD_BODY,NULL},
  {"admin","approve-affiliate-request","POST","/api/admin/affiliate-requests/:id/approve",1,PAYLOAD_BODY,NULL},
  {"admin","deny-affiliate-request","POST","/api/admin/affiliate-requests/:id/deny",1,PAYLOAD_BODY,NULL},
  {"admin","user","GET","/api/admin/users/:id",1,PAYLOAD_QUERY,NULL},
  {"admin","verify","PATCH","/api/admin/users/:id/verify",1,PAYLOAD_BODY,NULL},
  {"admin","gold","PATCH","/api/admin/users/:id/gold",1,PAYLOAD_BODY,NULL},
  {"admin","gray","PATCH","/api/admin/users/:id/gray",1,PAYLOAD_BODY,NULL},
  {"admin","outlines","PATCH","/api/admin/users/:id/outlines",1,PAYLOAD_BODY,NULL},
  {"admin","set-avatar","POST","/api/admin/users/:id/avatar",1,PAYLOAD_FILE,"avatar"},
  {"admin","delete-avatar","DELETE","/api/admin/users/:id/avatar",1,PAYLOAD_NONE,NULL},
  {"admin","set-banner","POST","/api/admin/users/:id/banner",1,PAYLOAD_FILE,"banner"},
  {"admin","delete-banner","DELETE","/api/admin/users/:id/banner",1,PAYLOAD_NONE,NULL},
  {"admin","mass-follow","POST","/api/admin/users/:id/mass-follow",1,PAYLOAD_BODY,NULL},
  {"admin","permissions","GET","/api/admin/users/:id/permissions",1,PAYLOAD_QUERY,NULL},
  {"admin","set-permissions","PATCH","/api/admin/users/:id/permissions",1,PAYLOAD_BODY,NULL},
  {"admin","badges","GET","/api/admin/badges",0,PAYLOAD_QUERY,NULL},
  {"admin","create-badge","POST","/api/admin/badges",0,PAYLOAD_BODY,NULL},
  {"admin","update-badge","PATCH","/api/admin/badges/:id",1,PAYLOAD_BODY,NULL},
  {"admin","delete-badge","DELETE","/api/admin/badges/:id",1,PAYLOAD_NONE,NULL},
  {"admin","user-badges","GET","/api/admin/users/:id/badges",1,PAYLOAD_QUERY,NULL},
  {"admin","assign-badge","POST","/api/admin/users/:id/badges",1,PAYLOAD_BODY,NULL},
  {"admin","remove-badge","DELETE","/api/admin/users/:id/badges/:badgeId",2,PAYLOAD_NONE,NULL},
  {"admin","suspend","POST","/api/admin/users/:id/suspend",1,PAYLOAD_BODY,NULL},
  {"admin","unsuspend","POST","/api/admin/users/:id/unsuspend",1,PAYLOAD_BODY,NULL},
  {"admin","delete-user","DELETE","/api/admin/users/:id",1,PAYLOAD_NONE,NULL},
  {"admin","clone-user","POST","/api/admin/users/:id/clone",1,PAYLOAD_BODY,NULL},
  {"admin","posts","GET","/api/admin/posts",0,PAYLOAD_QUERY,NULL},
  {"admin","delete-post","DELETE","/api/admin/posts/:id",1,PAYLOAD_NONE,NULL},
  {"admin","suspensions","GET","/api/admin/suspensions",0,PAYLOAD_QUERY,NULL},
  {"admin","post","GET","/api/admin/posts/:id",1,PAYLOAD_QUERY,NULL},
  {"admin","update-post","PATCH","/api/admin/posts/:id",1,PAYLOAD_BODY,NULL},
  {"admin","change-post-id","PATCH","/api/admin/posts/:id/id",1,PAYLOAD_BODY,NULL},
  {"admin","create-tweet","POST","/api/admin/tweets",0,PAYLOAD_BODY,NULL},
  {"admin","update-user","PATCH","/api/admin/users/:id",1,PAYLOAD_BODY,NULL},
  {"admin","impersonate","POST","/api/admin/impersonate/:id",1,PAYLOAD_BODY,NULL},
  {"admin","dms","GET","/api/admin/dms",0,PAYLOAD_QUERY,NULL},
  {"admin","search-dms","GET","/api/admin/dms/search",0,PAYLOAD_QUERY,NULL},
  {"admin","dm","GET","/api/admin/dms/:id",1,PAYLOAD_QUERY,NULL},
  {"admin","dm-messages","GET","/api/admin/dms/:id/messages",1,PAYLOAD_QUERY,NULL},
  {"admin","delete-dm","DELETE","/api/admin/dms/:id",1,PAYLOAD_NONE,NULL},
  {"admin","delete-dm-message","DELETE","/api/admin/dms/messages/:id",1,PAYLOAD_NONE,NULL},
  {"admin","fake-notification","POST","/api/admin/fake-notification",0,PAYLOAD_BODY,NULL},
  {"admin","moderation-logs","GET","/api/admin/moderation-logs",0,PAYLOAD_QUERY,NULL},
  {"admin","target-logs","GET","/api/admin/moderation-logs/target/:id",1,PAYLOAD_QUERY,NULL},
  {"admin","moderator-logs","GET","/api/admin/moderation-logs/moderator/:id",1,PAYLOAD_QUERY,NULL},
  {"admin","emojis","GET","/api/admin/emojis",0,PAYLOAD_QUERY,NULL},
  {"admin","create-emoji","POST","/api/admin/emojis",0,PAYLOAD_BODY,NULL},
  {"admin","delete-emoji","DELETE","/api/admin/emojis/:id",1,PAYLOAD_NONE,NULL},
  {"admin","fact-check","GET","/api/admin/fact-check/:postId",1,PAYLOAD_QUERY,NULL},
  {"admin","create-fact-check","POST","/api/admin/fact-check/:postId",1,PAYLOAD_BODY,NULL},
  {"admin","delete-fact-check","DELETE","/api/admin/fact-check/:id",1,PAYLOAD_NONE,NULL},
  {"admin","reports","GET","/api/admin/reports",0,PAYLOAD_QUERY,NULL},
  {"admin","resolve-report","POST","/api/admin/reports/:id/resolve",1,PAYLOAD_BODY,NULL},
  {"admin","super-tweeter","PATCH","/api/admin/users/:id/super-tweeter",1,PAYLOAD_BODY,NULL},
  {"admin","super-tweet","PATCH","/api/admin/posts/:id/super-tweet",1,PAYLOAD_BODY,NULL},
  {"admin","user-ip","GET","/api/admin/users/:id/ip",1,PAYLOAD_QUERY,NULL},
  {"admin","ip-users","GET","/api/admin/ip/:ip/users",1,PAYLOAD_QUERY,NULL},
  {"admin","ban-ip","POST","/api/admin/ip/ban",0,PAYLOAD_BODY,NULL},
  {"admin","unban-ip","POST","/api/admin/ip/unban",0,PAYLOAD_BODY,NULL},
  {"admin","ip-bans","GET","/api/admin/ip/bans",0,PAYLOAD_QUERY,NULL},
  {"admin","mass-engage","POST","/api/admin/posts/:id/mass-engage",1,PAYLOAD_BODY,NULL},
  {"admin","mass-delete-posts","POST","/api/admin/posts/mass-delete",0,PAYLOAD_BODY,NULL},
  {"admin","user-blocks","GET","/api/admin/users/:id/blocks",1,PAYLOAD_QUERY,NULL},
  {"admin","user-blocked-by","GET","/api/admin/users/:id/blocked-by",1,PAYLOAD_QUERY,NULL},
  {"admin","blocks","GET","/api/admin/blocks",0,PAYLOAD_QUERY,NULL},
  {"admin","create-ip-ban","POST","/api/admin/ip-bans",0,PAYLOAD_BODY,NULL},
  {"admin","rate-limit","POST","/api/admin/users/:id/rate-limit",1,PAYLOAD_BODY,NULL},
  {"admin","captcha-exempt","POST","/api/admin/users/:id/captcha-exempt",1,PAYLOAD_BODY,NULL},
  {"admin","shop-products","GET","/api/admin/shop/products",0,PAYLOAD_QUERY,NULL},
  {"admin","delete-shop-product","DELETE","/api/admin/shop/products/:id",1,PAYLOAD_NONE,NULL},
  {"admin","shop-purchases","GET","/api/admin/shop/purchases",0,PAYLOAD_QUERY,NULL},
};

void list_routes(void) {
  for (size_t i = 0; i < sizeof(ROUTES) / sizeof(ROUTES[0]); i++) {
    printf("%-14s %-28s %s %s\n", ROUTES[i].group, ROUTES[i].action, ROUTES[i].method, ROUTES[i].path);
  }
}

const Route *find_route(const char *group, const char *action) {
  for (size_t i = 0; i < sizeof(ROUTES) / sizeof(ROUTES[0]); i++) {
    if (strcmp(ROUTES[i].group, group) == 0 && strcmp(ROUTES[i].action, action) == 0) return &ROUTES[i];
  }
  return NULL;
}

char *render_route_path(const Route *r, int argc, char **argv) {
  Buffer b = {0};
  b.data = xstrdup("");
  int argi = 3;
  for (const char *p = r->path; *p;) {
    if (*p == ':') {
      const char *start = ++p;
      while ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '_') p++;
      (void)start;
      const char *val = argi < argc ? argv[argi++] : "";
      size_t n = strlen(val);
      b.data = xrealloc(b.data, b.len + n + 1);
      memcpy(b.data + b.len, val, n);
      b.len += n;
      b.data[b.len] = 0;
    } else {
      b.data = xrealloc(b.data, b.len + 2);
      b.data[b.len++] = *p++;
      b.data[b.len] = 0;
    }
  }
  return b.data;
}

int cmd_named_route(Config *cfg, int argc, char **argv) {
  if (strcmp(argv[1], "routes") == 0 || strcmp(argv[1], "commands") == 0) {
    list_routes();
    return 0;
  }
  if (argc < 3) return 2;
  const Route *r = find_route(argv[1], argv[2]);
  if (!r) return 2;
  if (argc < 3 + r->path_args) {
    fprintf(stderr, "tweeta: %s %s requires %d positional argument(s)\n", r->group, r->action, r->path_args);
    return 2;
  }
  char *path = render_route_path(r, argc, argv);
  int opt_start = 3 + r->path_args;
  int rc;
  if (r->payload == PAYLOAD_QUERY) {
    CURL *curl = curl_easy_init();
    char *q = query_from_options(curl, argc, argv, opt_start);
    char *full = malloc(strlen(path) + strlen(q) + 1);
    if (!full) die("out of memory");
    strcpy(full, path);
    strcat(full, q);
    rc = http_request(cfg, r->method, full, NULL, NULL, NULL, NULL);
    curl_easy_cleanup(curl);
    free(q);
    free(full);
  } else if (r->payload == PAYLOAD_BODY) {
    char *json = json_object_from_options(argc, argv, opt_start);
    rc = http_request(cfg, r->method, path, json, "application/json", NULL, NULL);
    free(json);
  } else if (r->payload == PAYLOAD_FILE) {
    const char *file = opt_value(argc, argv, "--file", NULL);
    if (!file && opt_start < argc && strncmp(argv[opt_start], "--", 2) != 0) file = argv[opt_start];
    if (!file) {
      fprintf(stderr, "tweeta: %s %s requires --file PATH\n", r->group, r->action);
      free(path);
      return 2;
    }
    rc = http_request(cfg, r->method, path, NULL, NULL, r->file_field, file);
  } else {
    rc = http_request(cfg, r->method, path, NULL, NULL, NULL, NULL);
  }
  free(path);
  return rc;
}
