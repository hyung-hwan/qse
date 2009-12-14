#utests.awk
#author pierre.gaston <a.t> gmail.com

@include "levenshtein.awk"

function testlevdist(str1, str2, correctval,    testval) {
    testval = levdist(str1, str2)
    if (testval == correctval) {
        printf "%s:\tCorrect distance between '%s' and '%s'\n", testval, str1, str2
        return 1
    } else {
        print "MISMATCH on words '%s' and '%s' (wanted %s, got %s)\n", str1, str2, correctval, testval
        return 0
    }
}
BEGIN {
    testlevdist("kitten",    "sitting",   3)
    testlevdist("Saturday",  "Sunday",    3)
    testlevdist("acc",       "ac",    1)
    testlevdist("foo",       "four",      2)
    testlevdist("foo",       "foo",       0)
    testlevdist("cow",       "cat",       2)
    testlevdist("cat",       "moocow",    5)
    testlevdist("cat",       "cowmoo",    5)
    testlevdist("sebastian", "sebastien", 1)
    testlevdist("more",      "cowbell",   5)
    testlevdist("freshpack", "freshpak",  1)
    testlevdist("freshpak",  "freshpack", 1)
}

