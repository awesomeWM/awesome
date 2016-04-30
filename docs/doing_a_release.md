## How to do a release

* Edit `awesomeConfig.cmake` and choose a codename that is not already listed in [[Releases]]. jd always picked a song title, you should do the same.
* Commit this changes to `awesomeConfig.cmake`
* Add the release version and date to [[Releases]]
* Change the version in [[Template:Stable-version]]
* Git sign with `git tag -s vX.Y.Z -m 'awesome vX.Y.Z`'
* Push with `git push` and push tag with `git push origin vX.Y.Z`
* Run `make dist` to create tarballs.
* Copy tarballs inside `awesome.naquadah.org:/var/www/awesome.naquadah.org/download/`
* Edit `download.mdwn` from [http://git.naquadah.org/?p=awesome-www.git;a=summary the awesome www repository] to change the version information, links, etc
* Go into the 'src' submodule, and update it to vX.Y.Z with `git checkout vX.Y.Z`. This will be used to build and publish the documentation online.
* Commit `download.mdwn` and `src`.
* `git push` this
* Type `make push` in `awesome-www` to push changes to the website. Be careful to have `ikiwiki`, `asciidoc` and imagemagick for Perl (`perlmagick`, otherwise images get broken)
* Send a mail to `awesome@naquadah.org` with any amount of information and jokes inside.
* Change the topic on IRC

For the announcement mail, the following shell script might be handy:

    #!/bin/sh
    VER=$1
    PREV_VER=$2
    TAG="v$VER"
    PTAG="v$PREV_VER"
    REVS="$PTAG..$TAG"
    
    echo "awesome version $VER has been released. It is available from:"
    
    print_file()
    {
            EXT="$1"
            URL="http://awesome.naquadah.org/download/awesome-$VER.$EXT"
            FILE="/var/www/awesome.naquadah.org/download/awesome-$VER.$EXT"
            MD5=$(ssh prometheus.naquadah.org "md5sum $FILE" 2>/dev/null | cut -f1 -d ' ')
            SHA1=$(ssh prometheus.naquadah.org "sha1sum $FILE" 2>/dev/null | cut -f 1 -d ' ')
    
            echo
            echo "$EXT: $URL"
            echo "md5: $MD5"
            echo "sha1: $SHA1"
    }
    
    print_headline()
    {
            HEAD="$@"
            echo
            echo "$HEAD"
            echo "$HEAD" | sed -e 's/./-/g'
    }
    
    print_file "tar.xz"
    print_file "tar.bz2"
    
    print_headline "number of changes"
    git rev-list "$REVS" | wc -l
    
    print_headline "number of commiters"
    git log --format=format:%an "$REVS" | sort -u | wc -l
    
    print_headline "shortlog"
    git log "$REVS" | git shortlog --numbered | cat
    
    print_headline "diffstat"
    git diff --stat "$REVS" | cat
