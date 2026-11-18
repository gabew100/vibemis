# The test tree is intentionally opt-in.  The application and package builds
# do not enter it unless their qmake invocation explicitly adds CONFIG+=tests.
TEMPLATE = subdirs
CONFIG += ordered

contains(CONFIG, tests) {
    SUBDIRS += vrr \
               updater \
               bitrate \
               overlay \
               keyboard \
               native-deck
} else {
    message(VRR/updater/bitrate/keyboard/native-deck tests are disabled; rerun qmake with CONFIG+=tests)
}
