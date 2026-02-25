# Knights Safety/Security

This file contains some quick notes on safety/security risks in the
Knights game and mitigations that have been employed.

It is intended to be a living document, and will be updated as new
risks and/or mitigations are discovered.

If you have any comments on this document, please send them by email
to stephen@solarflare.org.uk. Thank you.

# Risks and Mitigations

 - Malicious Modifications to the Game Client
    - Risk: Since the game is multiplayer, and relies on peer-to-peer
      network connections, there is a risk that a player might modify
      their game client so as to send malicious packets to other
      players. This could include attempts to take over the user's PC
      (by exploiting security bugs in the game client), attempts to
      make the client display inappropriate or offensive content, or
      "denial of service" attacks (attempts to make the user's game
      unplayable by sending more packets than the client can process).
    - Mitigations: 
       - The network protocol code has been carefully reviewed; for
         example, all values received are checked to make sure they
         are within valid bounds. This should minimize the risk of any
         security bugs in the game client. Of course, if any security
         bugs were to be reported to the author, then they would be
         fixed as a priority.
       - The network protocol does not ever send any graphics or
         sounds across the network, eliminating any possibility for
         players to send offensive images or sounds to each other.
       - Care has also been taken not to trust arbitrary text strings
         coming from the server. Instead, "localization keys" are sent
         for in-game messages, which are then looked up in a local
         file on disk. This means that game messages cannot be hacked
         to display offensive messages; only messages already built in
         to the game client (in particular, `localization_*.txt`
         files) can be displayed.
       - If playing on Steam, chat messages are not routed through the
         peer-to-peer network connection, but instead routed through
         Steam's servers. This removes any possibility of a player
         modifying or spoofing chat messages, impersonating other
         players, etc. (Note this does not apply when playing in LAN
         mode, but presumably if one is playing on a LAN then one
         trusts the other players much more, so it is less of an
         issue.)
       - At this time we do not introduce any particular mitigations
         against denial of service attacks. These are less severe than
         the other forms of hacking, because they simply mean that the
         victim cannot play Knights -- they do not result in offensive
         content being displayed or the player's machine being
         hacked. Therefore, it seems reasonable simply to wait to see
         if such attacks become a problem in practice, and if so, to
         introduce appropriate mitigations at that time.

 - Chat Abuse
    - Risk: Players might mis-use chat to verbally abuse other
      players, write hate speech, harass others, post spam, or attempt
      to scam others (e.g. by posting links to fake websites).
    - Mitigations: 
       - When playing on the Internet, all chat goes through
         Valve's servers and players are identifiable by their
         Steam ID. Abusive players can be reported to Valve using
         the standard Steam reporting mechanisms, and players can
         block communication with other players if necessary
         (the Knights code respects such blocks and will not
         display messages from blocked players).
       - All chat also passes through Steam's built-in chat filter,
         which should block inappropriate words or phrases.
       - The Knights client also does not allow links posted in chat
         to be clicked on (instead a player would have to manually
         type out any link into their web browser). This, for example,
         makes it less likely that a user would follow a link posted
         in chat without reviewing it first.

 - User-Generated Content
    - Risk: The Knights game supports user-generated content (or
      "mods"). For example, players can add new graphics, sounds,
      dungeon maps, text strings, or other in-game content. Some of
      this content may be offensive or harmful in some way.
    - Mitigations: 
       - Mods are never installed automatically; the user must always
         consent to using a mod. This means that players are always
         aware of the risk they are taking when installing content
         made by another user.
       - We plan to use the Steam Workshop system for installing mods.
         This provides a degree of authentication, as Steam Workshop
         mods must be uploaded by an authenticated Steam user (they
         cannot be uploaded anonymously). Moreover, Steam itself
         includes various mechanisms for reviewing and reporting
         mods, so it is expected that inappropriate mods would be
         swiftly removed by Valve's moderators.

 - Cheating
    - Risk: Players might modify their game client so as to gain
      in-game advantages, such as gaining hidden information (such as
      the position of gems in the dungeon) that is inaccessible to
      other players, or giving themselves power-ups (such as
      invulnerability) without earning them through the usual in-game
      mechanisms.
    - Mitigations: 
       - It is very difficult to actually mitigate against cheating.
         For example, if a player snoops the game's memory to find out
         where the gems are, then there is not really anything we can
         do to prevent this.
       - For now, we accept this risk and rely on the community to
         police cheaters. The Knights community is small, and if a
         particular player is known or suspected to be cheating, it is
         assumed that this fact would become known to the community
         quite quickly. The offending player could potentially be
         reported to Valve, or players could simply refuse to play
         with that person. The fact that players cannot play
         anonymously, but must use a valid Steam account, helps with
         this.
