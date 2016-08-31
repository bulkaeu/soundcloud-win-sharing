# SoundCloud Desktop Sharing Kit for Microsoft Windows
## Introduction

The Desktop Sharing Kit is a simple way to add Sharing to SoundCloud to your desktop application.
It comes as a seperate executable for Microsoft Windows and Mac OS that you can include in your application and invoke it from there to let the user share a sound to SoundCloud.

![Screenshot](http://dl.dropbox.com/u/12477597/Permanent/DesktopSharing/win-sharing.png)

This README describes the Microsoft Windows version. Head over [here to the Mac OS X version](https://github.com/soundcloud/soundcloud-mac-sharing).

## Installation

You can either download the latest build from [GitHub](https://github.com/soundcloud/soundcloud-win-sharing/downloads)
or compile it from source yourself. To clone the repository:

$ git clone git://github.com/soundcloud/soundcloud-win-sharing.git

## Configuration

Head over to [SoundCloud to register an application](http://soundcloud.com/you/apps). If you plan to use the Windows version the redirect URI has to be set to
"ttps://developers.soundcloud.com/callback.html", so best use this one in general.


You'll have to pass the client_id, client_secret and redirect_uri later when invoking the executable.

## Usage

Once you've added executable to your application you can call it using it's command line interface:

    C:\>"Share on SoundCloud.exe" /client_id:YOUR_CLIENT_ID /track[asset_data]:audio.mp3 /track[title]:"Test Sound"

The arguments you can pass into the app are:

* ``client_id``: Manually pass in a client ID if you don't want to store it inside the application
* ``track[asset_data]``: The path to the sound
* ``track[title]``: The title
* ``track[license]``: The license
* ``track[tag_list]``: A space seperated list of tags
* ``track[artwork_data]``: The path to an artwork file

There are more options you can pass in using the track[...] arguments. See the complete list in the [SoundCloud developer documentation](http://developers.soundcloud.com/docs/api/tracks).

## Support

You're very welcome to fork this project and send us pull requests. Also if you're running into issues feel free to [reach out to us](http://developers.soundcloud.com/support).
