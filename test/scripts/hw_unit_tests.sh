if ./test/scripts/rebuild.sh; then
  echo "Built"
else
  echo "Build failed"
  exit 1
fi
./test/scripts/run_hw_unit_tests.sh
