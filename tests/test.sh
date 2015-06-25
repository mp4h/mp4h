#! /bin/sh

program=../../src/mp4h
flags="$*"

SUBDIRS='define-tag undef let function-def set-var subst-in-string subst-in-var increment symbol-info string-eq string-neq string-compare string-length downcase upcase capitalize char-offsets printf match array-shift array-size array-member array-add-unique array-push array-concat foreach sort comment include if ifeq ifneq when while var-case get-file-properties directory-contents file-exists real-path Misc attributes-remove attributes-extract attributes-quote divert entity'

total=0
failed=0
passed=0
for d in $SUBDIRS
do
    cd $d
    for f in *.in
    do
        total=`expr $total + 1`
        base=`echo $f | sed -e 's,\.in$,,'`
        $program $flags $f > $base.out 2>/dev/null
        if cmp $base.ok $base.out >/dev/null 2>&1; then
            passed=`expr $passed + 1`
            echo $d/$base | awk '{printf "%-30s [@@ok@@]\n", $1}' \
            | sed -e 's/ /./g' -e 's/@/ /g'
        else
            failed=`expr $failed + 1`
            echo $d/$base | awk '{printf "%-30s [FAILED]\n", $1}' \
            | sed -e 's/ /./g' -e 's/@/ /g'
        fi
    done
    cd ..
done
echo '=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-='
echo "    Tests processed : $total"
echo "             passed : $passed"
echo "             failed : $failed"
[ $failed -ne 0 ] || ../shtool echo -e "      %BAll tests successful%b" 
echo ''
